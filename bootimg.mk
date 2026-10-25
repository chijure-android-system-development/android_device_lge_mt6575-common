$(INSTALLED_BOOTIMAGE_TARGET): $(BOARD_CUSTOM_MKBOOTIMG) \
		$(INTERNAL_BOOTIMAGE_FILES)
	@echo -e ${CL_CYN}"----- Making boot image (MTK) ------"${CL_RST}
	$(hide) $(BOARD_CUSTOM_MKBOOTIMG) $(INTERNAL_BOOTIMAGE_ARGS) $(BOARD_MKBOOTIMG_ARGS) --mtk 1 --output $@
	$(hide) $(call assert-max-image-size,$@,$(BOARD_BOOTIMAGE_PARTITION_SIZE),raw)
	@echo -e ${CL_CYN}"Made boot image: $@"${CL_RST}

$(INSTALLED_RECOVERYIMAGE_TARGET): $(BOARD_CUSTOM_MKBOOTIMG) \
		$(recovery_ramdisk) \
		$(recovery_kernel)
	@echo -e ${CL_CYN}"----- Making recovery image (MTK) ------"${CL_RST}
	$(hide) $(BOARD_CUSTOM_MKBOOTIMG) $(INTERNAL_RECOVERYIMAGE_ARGS) $(BOARD_MKBOOTIMG_ARGS) --mtk 1 --output $@
	$(hide) $(call assert-max-image-size,$@,$(BOARD_RECOVERYIMAGE_PARTITION_SIZE),raw)
	@echo -e ${CL_CYN}"Made recovery image: $@"${CL_RST}
