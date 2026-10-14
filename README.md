# MediaTek device modules for gold

The vendor kernel modules of the Redmi Note 13 5G (`gold`, MT6833) for LineageOS, built together with the system. How to set up the build is in [local_manifests](https://github.com/Star-caorui/local_manifests/tree/lineage-24.0-oss).

| | |
| --- | --- |
| Base | Xiaomi's release of MediaTek's `kernel_device_modules-6.6`: [MiCode/MTK_kernel_device_modules](https://github.com/MiCode/MTK_kernel_device_modules) `spark-w-oss` (`ee2aaed`) |
| On top | one commit per change; the first removes what gold does not build |
| Display | Xiaomi's `mi_disp` from the REDMI Turbo 5 Max's 6.6 tree, MiCode `dash-w-oss`, with gold's changes |
| gold's own drivers | from gold's 5.10 tree, [MiCode/Xiaomi_Kernel_OpenSource](https://github.com/MiCode/Xiaomi_Kernel_OpenSource) `gold-s-oss` |
| Config | `kernel/configs/gold.config`, merged after `mgk_64_k66` and `mt6833_overlay` |
| Still stock | the modules listed in the device tree's [`kernel-prebuilt/Makefile`](https://github.com/Star-caorui/android_device_xiaomi_gold/blob/lineage-24.0-oss/kernel-prebuilt/Makefile) |
