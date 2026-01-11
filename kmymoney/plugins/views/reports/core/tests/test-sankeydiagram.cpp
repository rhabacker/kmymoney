#include <QApplication>
#include <QMainWindow>

#include <KChartCartesianCoordinatePlane>
#include <KChartChart>

#include "../../sankeydiagram.h"
#include "sankeytestmodel.h"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    auto* model = new SankeyTestModel;

    auto* diagram = new SankeyDiagram;
    diagram->setModel(model);

    auto* chart = new KChart::Chart;
    auto* plane = new KChart::CartesianCoordinatePlane(chart);
    plane->replaceDiagram(diagram);
    chart->replaceCoordinatePlane(plane);

    QMainWindow window;
    window.setCentralWidget(chart);
    window.resize(900, 600);
    window.show();

    return app.exec();
}
