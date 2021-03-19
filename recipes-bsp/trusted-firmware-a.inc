SECTION = "bootloaders"
SUMMARY = "Trusted Firmware-A for RZ/G2L"
LICENSE = "BSD-3-Clause"

inherit deploy

DEPENDS = "bootparameter-native u-boot fiptool-native"

S = "${WORKDIR}/git"

COMPATIBLE_MACHINE = "(rzg2l-smarc|smarc-rzg2l|rzg2l-eval)"

PLATFORM_smarc-rzg2l = "rzg2l"
EXTRA_FLAGS_smarc-rzg2l = "BOARD_TYPE=BOARD_RZG2L_EVA"
FLASH_ADDRESS_BL2_BP_smarc-rzg2l = "00000"
FLASH_ADDRESS_FIP_smarc-rzg2l = "1D200"

PLATFORM_rzg2l-eval = "rzg2l"
EXTRA_FLAGS_rzg2l-eval = "BOARD_TYPE=BOARD_RZG2L_15MMSQ"
FLASH_ADDRESS_BL2_BP_rzg2l-eval = "00000"
FLASH_ADDRESS_FIP_rzg2l-eval = "1D200"

PLATFORM = "rzg2l"

SRC_URI = "file://rzg2l_tf-a_flash-bin_template.ttl"

# Requires CROSS_COMPILE set by hand as there is no configure script
export CROSS_COMPILE="${TARGET_PREFIX}"

# Let the Makefile handle setting up the CFLAGS and LDFLAGS as it is a standalone application
CFLAGS[unexport] = "1"
LDFLAGS[unexport] = "1"
AS[unexport] = "1"
LD[unexport] = "1"

# No configure
do_configure[noexec] = "1"

do_compile () {
	# Build TF-A
	oe_runmake PLAT=${PLATFORM} ${EXTRA_FLAGS} bl2 bl31

	# Create bl2_bp.bin
	bootparameter build/${PLATFORM}/release/bl2.bin bl2_bp.bin
	cat build/${PLATFORM}/release/bl2.bin >> bl2_bp.bin

	# Create fip.bin
	fiptool create --align 16 --soc-fw build/${PLATFORM}/release/bl31.bin --nt-fw ${DEPLOY_DIR_IMAGE}/u-boot.bin fip.bin

	# Convert to srec
	objcopy -O srec --adjust-vma=0x00011E00 --srec-forceS3 -I binary bl2_bp.bin bl2_bp.srec
	objcopy -I binary -O srec --adjust-vma=0x0000 --srec-forceS3 fip.bin fip.srec

	# Update flashing macro
	cp ${WORKDIR}/rzg2l_tf-a_flash-bin_template.ttl ${WORKDIR}/rzg2l_tf-a_flash-bin.ttl

	PROGRAM_SIZE_BL2_BP=$(printf "%x" `wc -c < bl2_bp.bin`)
	PROGRAM_SIZE_FIP=$(printf "%x" `wc -c < fip.bin`)

	sed -i "s|PROGRAM_SIZE_BL2_BP|${PROGRAM_SIZE_BL2_BP}|g" ${WORKDIR}/rzg2l_tf-a_flash-bin.ttl
	sed -i "s|PROGRAM_SIZE_FIP|${PROGRAM_SIZE_FIP}|g" ${WORKDIR}/rzg2l_tf-a_flash-bin.ttl
	sed -i "s|FLASH_ADDRESS_BL2_BP|${FLASH_ADDRESS_BL2_BP}|g" ${WORKDIR}/rzg2l_tf-a_flash-bin.ttl
	sed -i "s|FLASH_ADDRESS_FIP|${FLASH_ADDRESS_FIP}|g" ${WORKDIR}/rzg2l_tf-a_flash-bin.ttl
	sed -i "s|FILENAME_BL2_BP|bl2_bp-${MACHINE}.bin|g" ${WORKDIR}/rzg2l_tf-a_flash-bin.ttl
	sed -i "s|FILENAME_FIP|fip-${MACHINE}.bin|g" ${WORKDIR}/rzg2l_tf-a_flash-bin.ttl
}

do_deploy () {
	# Create deploy folder
	install -d ${DEPLOYDIR}

	# Copy IPL
	install -m 0644 ${S}/build/${PLATFORM}/release/bl2/bl2.elf ${DEPLOYDIR}/bl2-${MACHINE}.elf
	install -m 0644 ${S}/build/${PLATFORM}/release/bl2.bin ${DEPLOYDIR}/bl2-${MACHINE}.bin
	install -m 0644 ${S}/build/${PLATFORM}/release/bl31/bl31.elf ${DEPLOYDIR}/bl31-${MACHINE}.elf
	install -m 0644 ${S}/build/${PLATFORM}/release/bl31.bin ${DEPLOYDIR}/bl31-${MACHINE}.bin

	# Copy fip images
	install -m 0644 ${S}/bl2_bp.bin ${DEPLOYDIR}/bl2_bp-${MACHINE}.bin
	install -m 0644 ${S}/bl2_bp.srec ${DEPLOYDIR}/bl2_bp-${MACHINE}.srec
	install -m 0644 ${S}/fip.bin ${DEPLOYDIR}/fip-${MACHINE}.bin
	install -m 0644 ${S}/fip.srec ${DEPLOYDIR}/fip-${MACHINE}.srec

	# Copy flashing macro
	install -m 0644 ${WORKDIR}/rzg2l_tf-a_flash-bin.ttl ${DEPLOYDIR}/rzg2l_tf-a_flash-bin-${MACHINE}.ttl
}

addtask deploy before do_build after do_compile