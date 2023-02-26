#include <iostream>
#include <vector>
#include <unordered_map>
#include <cstring>
#include <memory>
#include <functional>
#include <iterator>

namespace Drawer
{
    /*!
         \brief Интерфейс к объекту с базовыми методами отрисовки
     */
    class IDrawer
    {
    public:
        virtual ~IDrawer() = default;
        virtual void drawCircle(double centerX, double centerY, double radius) const = 0;
        virtual void drawPoligon(const std::vector<double> &points) const = 0;
    };

    /*!
         \brief Объект-рисовальщик
     */
    class Drawer : public IDrawer
    {
    public:
        void drawCircle(double /*centerX*/, double /*centerY*/, double /*radius*/) const
        {
           // ...
        }
        void drawPoligon(const std::vector<double> &/*points*/) const
        {
           // ...
        }
    };
}

namespace Figure
{
    /*!
         \brief Типы фигур
     */
    enum Type
    {
        eCircle,
        eTriangle,
        eSquare
    };

    /*!
         \brief Базовый класс фигуры
     */
    class Figure
    {
        Type _type;
        size_t _countParams;

    public:
        Figure(Type type_, size_t countParams_)
        : _type(type_),
          _countParams(countParams_)
        {
        }
        virtual ~Figure() = default;
        Type type() const { return _type; };
        size_t countParams() { return _countParams; }

        virtual void draw(const Drawer::IDrawer &drawer, const std::vector<double> &params) const = 0;
    };

    /*!
         \brief Реализация фигуры круг
     */
    class Circle : public Figure
    {
        static const size_t COUNT_PARAMS = 3;
    public:
        Circle() : Figure(TYPE, COUNT_PARAMS) {}
        virtual ~Circle() = default;
        void draw(const Drawer::IDrawer &drawer, const std::vector<double> &params) const override
        {
            if (params.size() >= COUNT_PARAMS)
                drawer.drawCircle(params[0], params[1], params[2]);
        }
        static const Type TYPE = eCircle;
    };

    /*!
         \brief Реализация фигуры треугольник
     */
    class Triangle : public Figure
    {
        static const size_t COUNT_PARAMS = 6;
    public:
        Triangle() : Figure(TYPE, COUNT_PARAMS) {}
        virtual ~Triangle() = default;
        void draw(const Drawer::IDrawer &drawer, const std::vector<double> &params) const override
        {
            if (params.size() >= COUNT_PARAMS)
                drawer.drawPoligon(params);
        }
        static const Type TYPE = eTriangle;
    };

    /*!
         \brief Реализация фигуры квадрат
     */
    class Square : public Figure
    {
        static const size_t COUNT_PARAMS = 8;
    public:
        Square() : Figure(TYPE, COUNT_PARAMS) {}
        virtual ~Square() = default;
        void draw(const Drawer::IDrawer &drawer, const std::vector<double> &params) const override
        {
            if (params.size() >= COUNT_PARAMS)
                drawer.drawPoligon(params);
        }
        static const Type TYPE = eSquare;
    };

    /*!
         \brief Фабрика для генерации объектов фигур
     */
    class Factory
    {
        std::unordered_map<Type, std::function<Figure*()>> figureFactory;

        template <typename FigureImpl>
        void registerFigure(bool &res)
        {
            const Type type = FigureImpl::TYPE;
            auto it = figureFactory.find(type);
            if (it != figureFactory.end())
            {
                res = false;
                return;
            }
            figureFactory.emplace(type, []() -> Figure* { return new FigureImpl(); });
        }
    public:
        virtual ~Factory() = default;
        template <typename... Args>
        bool registerFigure()
        {
            bool res = true;
            int dummy[] = { 0, (registerFigure<Args>(res), 0)... };
            (void)dummy;
            return res;
        }
        Figure* createFigure(Type type) const
        {
            auto it = figureFactory.find(type);
            if (it == figureFactory.end())
            {
                return nullptr;
            }
            const auto &creator = it->second;
            return creator();
        }
    };
}

namespace Reader
{
    /*!
         \brief Интерфейс к объекту читателю
     */
    class IReader
    {
    public:
        virtual ~IReader() = default;
        virtual bool read(void *dst, size_t size, size_t count = 1) const = 0;
    };
    /*!
         \brief Чтение данных из файла
     */
    class File : public IReader
    {
        std::unique_ptr<FILE, std::function<void(FILE*)>> file;
    public:
        explicit File(const std::string &filename)
            : file(::fopen(filename.c_str(), "rb"), [](FILE* f) { ::fclose(f); })
        {
        }
        bool read(void *dst, size_t size, size_t count = 1) const
        {
            if (!dst || !file)
                return false;

            return ::fread(dst, size, count, file.get()) == count;
        }
    };
}

/*!
     \brief Класс, реализующий чтение данных, создание объекта фигуры и отрисовку
 */
class Feature
{
    const Figure::Factory &figureFactory;
    std::unordered_map<Figure::Type, std::shared_ptr<Figure::Figure>> figures;

    Figure::Figure *currentFigure = nullptr;
    std::vector<double> currentParams;

public:
    Feature(const Figure::Factory &figureFactory)
        : figureFactory(figureFactory)
    {
    }
    bool read(const Reader::IReader &reader)
    {
        Figure::Type type;
        if (!reader.read(&type, sizeof(type)))
            return false;

        auto it = figures.find(type);
        if (it == figures.end())
        {
            Figure::Figure *figure = figureFactory.createFigure(type);
            if (!figure)
                return false;
            it = figures.emplace(type, std::shared_ptr<Figure::Figure>(figure)).first;
        }

        Figure::Figure *figure = it->second.get();

        using paramType = decltype(currentParams)::value_type;
        std::vector<paramType> params(figure->countParams());
        if (!reader.read(params.data(), sizeof(paramType), figure->countParams()))
            return false;

        currentFigure = figure;
        currentParams = params;
        return true;
    }
    void draw(const Drawer::IDrawer &drawer)
    {
        if (currentFigure)
            currentFigure->draw(drawer, currentParams);
    }
    bool isValid()
    {
        return !!currentFigure;
    }
};

namespace Testing
{
    /*!
         \brief Тестовый mock объект для чтения данных
     */
    class ReaderMock : public Reader::IReader
    {
    public:
        bool read(void *dst, size_t /*size*/, size_t count = 1) const override
        {
            if (count == 1) // reading of type
            {
                const Figure::Type value = Figure::eCircle;
                memcpy(dst, &value, sizeof(value));
                std::cout << "ReaderMock::read(): type: eCircle" << std::endl;
                return true;
            }

            // reading of params
            std::cout << "ReaderMock::read(): params: { ";
            const double value = 2.1;
            while(count--)
            {
                std::cout << value << " ";
                memcpy(dst, &value, sizeof(value));
                dst = reinterpret_cast<uint8_t*>(dst) + sizeof(value);
            }
            std::cout << "} " << std::endl;
            return true;
        }
    };

    /*!
         \brief Тестовый mock объект для отрисовки
     */
    class DrawerMock : public Drawer::IDrawer
    {
    public:
        void drawCircle(double centerX, double centerY, double radius)  const override
        {
            std::cout << "DrawerMock::drawCircle():"
                      << " centerX = " << centerX
                      << " centerY = " << centerY
                      << " radius = "  << radius
                      << std::endl;
        }

        void drawPoligon(const std::vector<double> &points)  const override
        {
            std::cout << "DrawerMock::drawPoligon(): params: { ";
            std::copy(points.cbegin(), points.cend(), std::ostream_iterator<double>(std::cout, " "));
            std::cout << "} " << std::endl;
        }
    };
}

#define TestMode 0

int main()
{
    Figure::Factory figureFactory;
    figureFactory.registerFigure<Figure::Circle, Figure::Triangle, Figure::Square>();

#if not TestMode
    Reader::File reader("features.dat");
    Drawer::Drawer drawer;
#else
    Testing::ReaderMock reader;
    Testing::DrawerMock drawer;
#endif

    Feature feature(figureFactory);

    feature.read(reader);
    feature.draw(drawer);

    if (!feature.isValid())
        return 1;

    return 0;
}
