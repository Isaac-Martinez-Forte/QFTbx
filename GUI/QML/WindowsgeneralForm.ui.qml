import QtQuick 2.4
import QtQuick.Controls 1.3
import QtQuick.Layouts 1.1

Item {
    id: item1
    width: 735
    height: 146

    property alias defPlanta: defPlanta
    property alias especificaciones: especificaciones
    property alias frecuencias: frecuencias
    property alias templates: templates
    property alias boundaries: boundaries
    property alias estControlador: estControlador
    property alias loopShapint: loopShapint

    property alias barraDeProgreso: barraDeProgreso

    Column {
        id: column1
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.fill: parent

        RowLayout {
            width: 718
            height: 45
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 39
            anchors.verticalCenterOffset: 12
            anchors.horizontalCenterOffset: 0
            anchors.centerIn: parent

            Button {
                width: 84
                height: 40
                id: defPlanta
                text: "Definición\nplanta"
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: false
            }

            Button {
                id: especificaciones
                text: qsTr("Especificaciones")
                enabled: false
            }

            Button {
                id: frecuencias
                text: qsTr("Frecuencias")
            }

            Button {
                id: templates
                text: qsTr("Templates")
                enabled: false
            }

            Button {
                id: boundaries
                text: qsTr("Boundaries")
                enabled: false
            }

            Button {
                width: 89
                height: 40
                id: estControlador
                text: "Estructura\ncontrolador"
                enabled: false
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            Button {
                width: 63
                height: 40
                id: loopShapint
                text: qsTr("Ajuste\ndel lazo")
                enabled: false
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }

        ProgressBar {
            id: barraDeProgreso
            width: 718
            height: 23
            anchors.top: parent.top
            anchors.topMargin: 0
        }
    }
}
