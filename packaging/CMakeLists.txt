install(PROGRAMS
    launchers/linux-thermal-control
    DESTINATION /usr/bin
)
install(PROGRAMS
    launchers/linux-thermal-daemon
    DESTINATION /usr/libexec/linux-thermal-control
)
install(PROGRAMS
    ${PROJECT_SOURCE_DIR}/tools/linux-thermal-recovery
    DESTINATION /usr/libexec/linux-thermal-control
)
install(FILES
    systemd/linux-thermal-daemon.service
    DESTINATION /usr/lib/systemd/system
)
install(FILES
    dbus/system.d/org.linuxthermal.Control1.conf
    DESTINATION /usr/share/dbus-1/system.d
)
install(FILES
    dbus/system-services/org.linuxthermal.Control1.service
    DESTINATION /usr/share/dbus-1/system-services
)
install(FILES
    polkit/org.linuxthermal.control.policy
    DESTINATION /usr/share/polkit-1/actions
)
install(FILES
    ${PROJECT_SOURCE_DIR}/config/default-fan-curve.yaml
    DESTINATION /etc/linux-thermal-control
)
install(FILES
    desktop/org.linuxthermal.Control.desktop
    DESTINATION /usr/share/applications
)
install(FILES
    icons/org.linuxthermal.Control.svg
    DESTINATION /usr/share/icons/hicolor/scalable/apps
)
install(FILES
    ${PROJECT_SOURCE_DIR}/docs/CPU风扇控制说明.md
    ${PROJECT_SOURCE_DIR}/docs/开发环境.md
    ${PROJECT_SOURCE_DIR}/docs/架构设计.md
    ${PROJECT_SOURCE_DIR}/LICENSE
    debian/copyright
    DESTINATION /usr/share/doc/linux-thermal-control
)
install(FILES
    man/linux-thermal-control.1
    DESTINATION /usr/share/man/man1
)
install(FILES
    debian/changelog
    DESTINATION /usr/share/doc/linux-thermal-control
    RENAME changelog.Debian
)
install(FILES
    lintian/linux-thermal-control
    DESTINATION /usr/share/lintian/overrides
)
