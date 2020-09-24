import QtQuick 2.8
import QtQuick.Window 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import Qt.labs.platform 1.1

Window {
    visible: true
    width: 800
    height: 600
    title: qsTr("Figma")

    Grid {
        columns: 3
        Repeater {
            model: images.count
            Image {
                source: "image://" + images.provider + "/" + modelData
                width: 100
                height: 100
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        saveDialog.proposedFilename = images.name(modelData);
                        saveDialog.open()
                    }
                }
            }
        }
    }

    FileDialog {
        id: saveDialog
        property string proposedFilename
        title: "Save " + proposedFilename
        currentFile: "file:///" + encodeURIComponent(proposedFilename)
        folder: StandardPaths.writableLocation(StandardPaths.DocumentsLocation)
        nameFilters: [ "QML files (*.qml)", "All files (*)" ]
        fileMode: FileDialog.SaveFile
        onAccepted: {
            let path = saveDialog.file.toString();
            path = path.replace(/^(file:\/\/)/,"");
            if(!images.save(sourceChooser.text, path)) {
                console.log("Cannot save \"" + path + "\"")
            }
        }
    }
}
