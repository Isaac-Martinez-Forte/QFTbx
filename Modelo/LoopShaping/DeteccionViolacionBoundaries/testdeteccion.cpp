#include "TestDeteccion.h"

using namespace std;

TestDeteccion::TestDeteccion()
{

}

TestDeteccion::~TestDeteccion()
{

}

void TestDeteccion::setUp(DatosBound * &boundaries, QVector<QMap<QString, QVector<QPoint> *> *> * metadatos)
{
    AlgoritmoInterseccionProyectivo2D * pintor = new AlgoritmoInterseccionProyectivo2D();
    pintor->ejecutarAlgoritmo(boundaries, metadatos);
    interseccionesBoundaries2D = pintor->getInterseccionesBoundaries();
    AlgoritmoInterseccionLineal1D * lineal = new AlgoritmoInterseccionLineal1D();
    lineal->ejecutarAlgoritmo(boundaries, metadatos);
    interseccionesBoundaries1D = lineal->getInterseccionesBoundaries();
    //cout << "Prueba Lectura Primer Punto Interseccion Tests: " << interseccionesBoundaries1D->at(0)->at(0)->at(0).y() << endl;
}

void TestDeteccion::testMillon1D(DatosBound * &boundaries)
{
    DeteccionViolacionBoundaries1D * deteccion = new DeteccionViolacionBoundaries1D();
    double totalFase = boundaries->getTamFas();
    bool abierta = true, arriba = true;
    qint32 unMillon = 1000000;
    while(unMillon)
    {
        for(qreal i = -360;i<0;i++)
        {
            QPointF punto(i,0);
            deteccion->deteccionViolacion(punto, interseccionesBoundaries1D->at(0), totalFase, abierta, arriba);
        }
        unMillon--;
    }
}

void TestDeteccion::testMillon2D(DatosBound * &boundaries)
{
    DeteccionViolacionBoundaries2D * deteccion = new DeteccionViolacionBoundaries2D();
    qint32 totalMagnitud = boundaries->getTamMag();
    qint32 totalFase = boundaries->getTamFas();
    qint32 minDB = boundaries->getDatosMag().x();
    qint32 unMillon = 1000000;
    while(unMillon)
    {
        for(qreal i = -360;i<0;i++)
        {
            QPointF punto(i,0);
            deteccion->deteccionViolacion(punto, interseccionesBoundaries2D->at(0), totalFase, totalMagnitud, minDB);
        }
        unMillon--;
    }
}

void TestDeteccion::ejecutarTests(DatosBound * boundaries, QVector<QMap<QString, QVector<QPoint> *> *> * metadatos)
{
    setUp(boundaries, metadatos);
    QElapsedTimer timer;
    timer.start();
    testMillon1D(boundaries);
    cout << "Test 360 millones de Detecciones 1D: " << timer.elapsed() << " milliseconds" << endl;
    timer.restart();
    testMillon2D(boundaries);
    cout << "Test 360 millones de Detecciones 2D: " << timer.elapsed() << " milliseconds" << endl;
}
