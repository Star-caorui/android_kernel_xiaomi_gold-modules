# MediaTek device modules for gold

MediaTek's `kernel_device_modules-6.6` for the Redmi Note 13 5G (`gold`, MT6833), with gold's changes, as stock OS3.0.5.0.VNQMIXM builds its vendor modules. It is checked out at `kernel_device_modules-6.6`, next to the kernel, where stock's modules were built from.

| | |
| --- | --- |
| Base | Xiaomi's release of MediaTek's `kernel_device_modules-6.6` for the Redmi Pad SE: [MiCode/MTK_kernel_device_modules](https://github.com/MiCode/MTK_kernel_device_modules) `spark-w-oss` (`ee2aaed`) |
| On top | one commit per driver or change; a file taken from another MiCode branch is named with its branch |
| gold's own drivers | from gold's 5.10 tree, [MiCode/Xiaomi_Kernel_OpenSource](https://github.com/MiCode/Xiaomi_Kernel_OpenSource) `gold-s-oss` |
| Config | `kernel/configs/gold.config`, merged after `mgk_64_k66_defconfig` and `mt6833_overlay.config` |
| Device tree | `arch/arm64/boot/dts/mediatek/mt6833.dts`, which builds into stock's `gold.dtb`, and the overlay `gold.dtso` with `gold/cust.dtsi`, which build into stock's `dtbo`. The 6.6 overlay is not published: these are written from stock's `dtbo` |
| Build (`lineage-24.0`) | Kleaf, as LineageOS's lamu: `build.config.gold`, `build_mgk_64_k66.sh`, and `kernel/kleaf/gold_modules.bzl`, the modules gold's configuration builds |

Huaqin's drivers are in [android_kernel_xiaomi_gold-huaqin_modules](https://github.com/Star-caorui/android_kernel_xiaomi_gold-huaqin_modules), MediaTek's other modules in [android_kernel_xiaomi_gold-mediatek_modules](https://github.com/Star-caorui/android_kernel_xiaomi_gold-mediatek_modules).

`OS3.0.5.0.VNQMIXM` holds the sources of stock's modules, built with make the way stock's were; `lineage-24.0` is what the LineageOS build uses, with its own changes on top, built with Kleaf. The manifests of both, and how the modules compare with stock's, are in [android_kernel_xiaomi_gold_manifest](https://github.com/Star-caorui/android_kernel_xiaomi_gold_manifest).
