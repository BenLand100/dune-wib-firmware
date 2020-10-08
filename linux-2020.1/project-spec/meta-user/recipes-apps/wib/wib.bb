SUMMARY = "WIB applications"
LICENSE="CLOSED"
SECTION = "PETALINUX/apps"

DEPENDS = "glibc zeromq cppzmq protobuf protobuf-native readline"

# -compiler and -dev packages are necessary only for building on the wib itself
RDEPENDS_${PN} = "protobuf-compiler protobuf zeromq-dev cppzmq-dev protobuf-dev readline-dev git tar rsync"

#To disable hard failures from including -dev packages
INSANE_SKIP_${PN} += "dev-deps"

SRC_URI = "file://* \
          "
TARGET_CC_ARCH += "${LDFLAGS}"

#To rebuild wib sw every time
do_fetch[nostamp] = "1"
do_compile[nostamp] = "1"
do_install[nostamp] = "1"

S = "${WORKDIR}"
do_compile() {
    make clean
    oe_runmake
}
do_install() {
    install -d ${D}/bin/
    install -m 0755 ${S}/wib_server ${D}/bin/
    install -m 0755 ${S}/wib_client ${D}/bin/
    install -m 0755 ${S}/extras/wib_update.sh ${D}/bin/
    install -d ${D}/etc/rc5.d/
    install -m 0755 ${S}/extras/wib_init.sh ${D}/etc/rc5.d/S99wib_init.sh
}

