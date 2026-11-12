import QtQuick
import QtTest
import LinuxThermal.Control

Item {
    id: root
    width: 800
    height: 600
    Component { id: popupComponent; AppearancePopup {} }

    TestCase {
        name: "AppearancePopup"
        when: windowShown
        property int savedThemeMode

        function init() {
            savedThemeMode = AppSettings.themeMode
            AppSettings.themeMode = 1
        }
        function cleanup() {
            AppSettings.themeMode = savedThemeMode
        }
        function test_keyboardThemeSelection() {
            const popup = createTemporaryObject(popupComponent, root)
            verify(!!popup, "Component exists")
            popup.open()
            tryCompare(popup, "opened", true)
            const selector = findChild(popup, "themeSelector")
            verify(!!selector, "Object exists")
            selector.forceActiveFocus()
            keyClick(Qt.Key_Down)
            keyClick(Qt.Key_Return)
            tryCompare(AppSettings, "themeMode", 2)
            compare(Theme.dark, true)
            keyClick(Qt.Key_Escape)
            tryCompare(popup, "visible", false)
        }
    }
}
