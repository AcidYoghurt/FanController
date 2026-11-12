import QtQuick
import QtTest
import LinuxThermal.Control

Item {
    id: root
    width: 800
    height: 600
    Component {
        id: buttonComponent
        ModeButton { x: 20; y: 20; width: 320; height: 180; mode: "automatic"; title: qsTr("自动") }
    }
    TestCase {
        name: "ModeMotion"
        when: windowShown
        function test_hoverAndPress() {
            mouseMove(root, 700, 500)
            const button = createTemporaryObject(buttonComponent, root)
            verify(!!button, "Component exists")
            mouseMove(button, button.width / 2, button.height / 2)
            tryVerify(() => button.scale > 1)
            mousePress(button, button.width / 2, button.height / 2)
            tryVerify(() => button.scale < 1)
            mouseRelease(button, button.width / 2, button.height / 2)
            tryCompare(button, "scale", 1.012)
            mouseMove(root, 700, 500)
            tryCompare(button, "scale", 1)
        }
    }
}
