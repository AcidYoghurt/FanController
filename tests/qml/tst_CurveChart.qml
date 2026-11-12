import QtQuick
import QtTest
import LinuxThermal.Control

Item {
    id: root
    width: 800
    height: 600
    Component {
        id: chartComponent
        CurveChart {
            width: 500
            height: 320
            points: [{temperature: 30, duty: 20}, {temperature: 60, duty: 40}, {temperature: 100, duty: 100}]
        }
    }
    TestCase {
        name: "CurveMotion"
        when: windowShown
        function test_editMorphPreservesActualPoints() {
            const chart = createTemporaryObject(chartComponent, root)
            verify(!!chart, "Component exists")
            tryCompare(chart, "revealProgress", 1)
            chart.points = [{temperature: 30, duty: 20}, {temperature: 60, duty: 80}, {temperature: 100, duty: 100}]
            compare(chart.points[1].duty, 80)
            tryVerify(() => chart.displayedPoints[1].duty > 40 && chart.displayedPoints[1].duty < 80)
            tryVerify(() => chart.displayedPoints[1].duty === 80)
            chart.presentationActive = false
            compare(chart.revealProgress, 0)
            chart.presentationActive = true
            tryVerify(() => chart.revealProgress > 0 && chart.revealProgress < 1)
            tryCompare(chart, "revealProgress", 1)
        }
    }
}
