import QtQuick
import QtTest
import LinuxThermal.Control

Item {
    id: root
    width: 800
    height: 600
    Component { id: navigationComponent; HeaderNavigation { width: 204; height: 42; currentIndex: 0 } }
    TestCase {
        name: "NavigationMotion"
        when: windowShown
        function test_slidingSelection() {
            const navigation = createTemporaryObject(navigationComponent, root)
            verify(!!navigation, "Component exists")
            const marker = findChild(navigation, "navigationIndicator")
            verify(!!marker, "Object exists")
            const infoButton = findChild(navigation, "infoNavigationButton")
            verify(!!infoButton, "Object exists")
            navigation.pageRequested.connect(page => navigation.currentIndex = page)
            mouseClick(infoButton, infoButton.width / 2, infoButton.height / 2)
            tryCompare(infoButton, "selected", true)
            tryVerify(() => marker.x > 4 && marker.x < 104)
            tryCompare(marker, "x", 104)
            navigation.currentIndex = 0
            tryVerify(() => marker.x > 4 && marker.x < 104)
            tryCompare(marker, "x", 4)
        }
    }
}
