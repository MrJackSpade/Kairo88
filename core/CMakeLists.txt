# The Kairo88 PC-8801 core: QUASI88 (third_party/quasi88, unmodified) with the
# Kairo88 system layer in port/. Builds for Android (from the app's native
# build) and for the host (boot_test), so the core can be tested without a
# device.

cmake_minimum_required(VERSION 3.22)
project(kairo88_core C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(QUASI88_SRC "${CMAKE_CURRENT_LIST_DIR}/../third_party/quasi88/src")
set(YMFM_SRC "${CMAKE_CURRENT_LIST_DIR}/../third_party/ymfm/src")

# QUASI88's own sources, as its Makefile lists them, without the system
# (sysdepend/), OS (osdepend/) and sound-output (snddrv/) layers.
set(QUASI88_SOURCES
    alarm.c control.c debug.c emu.c fname.c getconf.c icon.c menu.c monitor.c pause.c quasi88.c utility.c
    mon/mon-arg.c mon/mon-cmd.c mon/mon-debug.c mon/mon-image.c mon/mon-memory.c mon/mon-misc.c mon/mon-set.c
    mon/mon-util.c
    pc88/basic.c pc88/crtcdmac.c pc88/fdc.c pc88/image.c pc88/intr.c pc88/keyboard.c pc88/memory.c pc88/pc88main.c
    pc88/pc88sub.c pc88/pio.c pc88/romaji.c pc88/soundbd.c pc88/suspend.c pc88/z80.c pc88/z80-debug.c
    screen/color.c screen/menu-screen.c screen/putimage.c screen/screen.c screen/screen-8bpp.c screen/screen-16bpp.c
    screen/screen-32bpp.c screen/screen-snapshot.c screen/snapshot.c
    tk/q8tk-draw.c tk/q8tk-event.c tk/q8tk-glib.c tk/q8tk.c
    tk/q8tk/q8tk-accel.c tk/q8tk/q8tk-adjustment.c tk/q8tk/q8tk-button.c tk/q8tk/q8tk-combo.c tk/q8tk/q8tk-common.c
    tk/q8tk/q8tk-container.c tk/q8tk/q8tk-entry.c tk/q8tk/q8tk-frame.c tk/q8tk/q8tk-fselection.c
    tk/q8tk/q8tk-fviewer.c tk/q8tk/q8tk-grab.c tk/q8tk/q8tk-item.c tk/q8tk/q8tk-label.c tk/q8tk/q8tk-listbox.c
    tk/q8tk/q8tk-misc.c tk/q8tk/q8tk-notebook.c tk/q8tk/q8tk-signal.c tk/q8tk/q8tk-widget.c tk/q8tk/q8tk-window.c
    ui/askopenfile-top.c ui/askquit-top.c ui/askreset-top.c ui/askselectdisk-top.c ui/askspeedup-top.c
    ui/askstatefile-top.c ui/menu-top.c ui/pause-top.c ui/statusbar.c ui/toolbar.c
    ui/menu/menu-about.c ui/menu/menu-common.c ui/menu/menu-cpu.c ui/menu/menu-disk.c ui/menu/menu-graph.c
    ui/menu/menu-key.c ui/menu/menu-misc.c ui/menu/menu-mouse.c ui/menu/menu-reset.c ui/menu/menu-tape.c
    ui/menu/menu-volume.c
    osdepend/unix/file-op.c)
list(TRANSFORM QUASI88_SOURCES PREPEND "${QUASI88_SRC}/")

add_library(kairo88_core STATIC
    ${QUASI88_SOURCES}
    port/graph.c port/event.c port/wait.c port/system.c
    sound/snddrv_ymfm.cpp
    "${YMFM_SRC}/ymfm_opn.cpp" "${YMFM_SRC}/ymfm_adpcm.cpp" "${YMFM_SRC}/ymfm_ssg.cpp" "${YMFM_SRC}/ymfm_misc.cpp"
    kairo88_core.c kairo88_loader.c)

# port/ comes first so its config.h and device.h stand in for a sysdepend port.
target_include_directories(kairo88_core
    PUBLIC include
    PRIVATE port "${QUASI88_SRC}"
    "${QUASI88_SRC}/osdepend" "${QUASI88_SRC}/sysdepend" "${QUASI88_SRC}/osdepend/unix" "${QUASI88_SRC}/mon"
    "${QUASI88_SRC}/pc88" "${QUASI88_SRC}/screen" "${QUASI88_SRC}/screen/func" "${QUASI88_SRC}/screen/func/macro"
    "${QUASI88_SRC}/tk" "${QUASI88_SRC}/tk/q8tk" "${QUASI88_SRC}/ui" "${QUASI88_SRC}/ui/menu" "${QUASI88_SRC}/snddrv"
    "${YMFM_SRC}")

# Directory defaults are unused: the host always passes the directories it needs.
target_compile_definitions(kairo88_core PRIVATE
    USE_SOUND ROM_DIR="" DISK_DIR="" TAPE_DIR="" SNAP_DIR="" STATE_DIR="" L_CFG_DIR="" G_CFG_DIR="" CLIB_DECL=)
target_compile_options(kairo88_core PRIVATE -w)
if(WIN32)
    target_compile_options(kairo88_core PRIVATE -include "${CMAKE_CURRENT_LIST_DIR}/port/windows_host.h")
endif()

if(NOT ANDROID)
    add_executable(boot_test test/boot_test.c)
    target_link_libraries(boot_test PRIVATE kairo88_core)
    add_executable(restart_test test/restart_test.c)
    target_link_libraries(restart_test PRIVATE kairo88_core)
    add_executable(tape_test test/tape_test.c)
    target_link_libraries(tape_test PRIVATE kairo88_core)
    if(WIN32)
        target_link_options(boot_test PRIVATE -static)
        target_link_options(restart_test PRIVATE -static)
        target_link_options(tape_test PRIVATE -static)
    endif()
endif()
