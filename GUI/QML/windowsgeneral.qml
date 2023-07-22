import QtQuick 2.4
import QtQuick.Controls 1.3
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2

ApplicationWindow {
    title: qsTr("Pantalla Principal")
    width: 740
    height: 121
    visible: true

    menuBar: MenuBar {
        Menu {
            title: qsTr("&Archivo")
            MenuItem {
                text: qsTr("&Nuevo")
                onTriggered: messageDialog.show(qsTr("Open action triggered"));
            }
            MenuItem {
                text: qsTr("&Abrir")
                onTriggered: messageDialog.show(qsTr("Open action triggered"));
            }

            MenuSeparator { }

            MenuItem {
                text: qsTr("&Guardar")
                onTriggered: messageDialog.show(qsTr("Open action triggered"));
            }
            MenuItem {
                text: qsTr("&Guardar Como")
                onTriggered: messageDialog.show(qsTr("Open action triggered"));
            }

            MenuSeparator { }

            MenuItem {
                text: qsTr("&Salir")
                onTriggered: Qt.quit();
            }
        }

        Menu {
            title: qsTr("&Ver")
            MenuItem {
                text: qsTr("&Consola")
                onTriggered: messageDialog.show(qsTr("Open action triggered"));
            }
        }

        Menu {
            title: qsTr("&Diagramas")
            MenuItem {
                text: qsTr("&Diagrama de Bode")
                onTriggered: messageDialog.show(qsTr("Open action triggered"));
            }
            Menu {
                title: qsTr("&Diagrama lazo")
                MenuItem {
                    text: qsTr("&Diagrama Lazo Nyquist")
                    onTriggered: messageDialog.show(qsTr("Open action triggered"));
                }
                MenuItem {
                    text: qsTr("&Diagrama Lazo Nichols")
                    onTriggered: messageDialog.show(qsTr("Open action triggered"));
                }
                MenuSeparator { }
                MenuItem {
                    text: qsTr("&Todos los diagramas")
                    onTriggered: messageDialog.show(qsTr("Open action triggered"));
                }
            }
        }
    }

    WindowsgeneralForm {
        x: 0
        y: 0
        width: 740
        height: 120
        defPlanta.onClicked: messageDialog.show(qsTr("Button 1 pressed"))
        especificaciones.onClicked: messageDialog.show(qsTr("Button 2 pressed"))
        frecuencias.onClicked: messageDialog.show(qsTr("Button 3 pressed"))
        templates.onClicked: messageDialog.show(qsTr("Button 1 pressed"))
        boundaries.onClicked: messageDialog.show(qsTr("Button 2 pressed"))
        estControlador.onClicked: messageDialog.show(qsTr("Button 3 pressed"))
        loopShapint.onClicked: messageDialog.show(qsTr("Button 1 pressed"))

    }
}
