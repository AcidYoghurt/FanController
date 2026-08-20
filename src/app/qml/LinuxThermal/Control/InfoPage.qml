import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ScrollView {
    id: root
    objectName: "infoPage"

    required property var backend

    contentWidth: availableWidth
    clip: true

    ColumnLayout {
        width: root.availableWidth
        spacing: Theme.sectionSpacing

        Item { implicitHeight: 4 }

        GridLayout {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.pageMargin
            Layout.rightMargin: Theme.pageMargin
            columns: root.availableWidth >= 980 ? 2 : 1
            columnSpacing: 20
            rowSpacing: 20

            SecureBootPanel {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                secureBootState: root.backend.secureBootState
                secureBootKnown: root.backend.secureBootKnown
                secureBootEnabled: root.backend.secureBootEnabled
                secureBootDisabled: root.backend.secureBootDisabled
            }

            FanPanel {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                fanModel: root.backend.fans
            }
        }

        BackendPanel {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.pageMargin
            Layout.rightMargin: Theme.pageMargin
            backendModel: root.backend.backends
        }

        Item { implicitHeight: 16 }
    }
}
