import QtQuick
import QtQuick.Controls
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

        PageHeading {
            Layout.fillWidth: true
            Layout.topMargin: 8
            Layout.leftMargin: Theme.pageMargin
            Layout.rightMargin: Theme.pageMargin
            title: qsTr("设备与安全")
            description: qsTr("检查设备能力、驱动后端与安全启动状态。")
            label: "SYSTEM / DETAILS"
        }

        GridLayout {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.pageMargin
            Layout.rightMargin: Theme.pageMargin
            columns: root.availableWidth >= 980 ? 2 : 1
            columnSpacing: 20
            rowSpacing: 20

            SecureBootPanel {
                entryDelay: 40
                viewport: root.contentItem
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                secureBootState: root.backend.secureBootState
                secureBootKnown: root.backend.secureBootKnown
                secureBootEnabled: root.backend.secureBootEnabled
                secureBootDisabled: root.backend.secureBootDisabled
                mokState: root.backend.secureBootMokState
                mokKnown: root.backend.secureBootMokKnown
                mokEnrolled: root.backend.secureBootMokEnrolled
            }

            FanPanel {
                entryDelay: 120
                viewport: root.contentItem
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                fanModel: root.backend.fans
            }
        }

        BackendPanel {
            entryDelay: 200
            viewport: root.contentItem
            Layout.fillWidth: true
            Layout.leftMargin: Theme.pageMargin
            Layout.rightMargin: Theme.pageMargin
            backendModel: root.backend.backends
        }

        Item { implicitHeight: 16 }
    }
}
