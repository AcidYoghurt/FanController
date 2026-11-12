import QtQuick
import QtTest
import LinuxThermal.Control

Item {
    id: root
    width: 800
    height: 600
    Component { id: panelComponent; Panel { width: 320; height: 180; entryDelay: 80 } }
    TestCase {
        name: "PanelMotion"
        when: windowShown
        function test_entryAndReturn() {
            const panel = createTemporaryObject(panelComponent, root)
            verify(!!panel, "Component exists")
            tryVerify(() => panel.opacity > 0 && panel.opacity < 1)
            tryCompare(panel, "entranceOffset", 0)
            tryCompare(panel, "opacity", 1)
            panel.visible = false
            compare(panel.opacity, 0)
            panel.visible = true
            tryVerify(() => panel.opacity > 0 && panel.opacity < 1)
            tryCompare(panel, "opacity", 1)
            tryCompare(panel, "entranceOffset", 0)
        }
    }
}
