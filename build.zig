const std = @import("std");

//zig build Ч соберет библиотеки libiio.so, libad9361.so, демона iiod и все 5 утилит. √отовые файлы по€в€тс€ в директори€х zig-out/lib/ и zig-out/bin/.
//zig build optimize ReleaseFast Ч соберет проект с максимальной оптимизацией (аналог -O3 в Makefile) (или ReleaseSafe дл€ -O2).
//zig build target aarch64-linux Ч кросс-компил€ци€ проекта под 64-битный Linux ARM (например, дл€ embedded-платформ).

//zig build target arm-linux-gnueabihf cpu cortex_a9 sysroot "C:\target_sysroot" -Doptimize ReleaseFast


fn configureCrossPaths(b: *std.Build, module: *std.Build.Module) void {

    module.addIncludePath(b.path("include"));
    module.addIncludePath(b.path("root_target/usr/include"));

    module.addLibraryPath(b.path("root_target/lib"));
    module.addLibraryPath(b.path("root_target/usr/lib"));

}



pub fn build(b: *std.Build) void {

    // ∆естко задаем таргет дл€ Zynq-7010 (armhf)
    const target = b.resolveTargetQuery(.{
        .cpu_arch = .arm,
        .os_tag = .linux,
        .abi = .gnueabihf,
        .cpu_model = .{ .explicit = &std.Target.arm.cpu.cortex_a9 },
        .glibc_version = std.SemanticVersion{ .major = 2, .minor = 25, .patch = 0 },
    });

    const optimize = b.standardOptimizeOption(.{});

    const compile_flags = &[_][]const u8{
        "-Wall", "-Wextra", "-Wno-unused-parameter", "-Wpedantic", "-Wshadow",
        "-DLargeFILE_SOURCE", "-DLargeFILE64_SOURCE", "-D_FILE_OFFSET_BITS=64",
        "-DNDEBUG", "-fvisibility=hidden", "-fpermissive", "-DLIBIIO_EXPORTS", "-D_GNU_SOURCE",
        "-Wno-zero-length-array","-Wno-gnu-folding-constant", 

    };

    // ---  Ѕ»ЅЋ»ќ“≈ ј libiio ---
    const libiio = b.addLibrary(.{
        .name = "iio",
        .linkage = .dynamic, 
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
        }),
    });
    
    
    libiio.root_module.linkSystemLibrary("c++", .{});
    libiio.root_module.linkSystemLibrary("avahi-client", .{ .use_pkg_config = .no });
    libiio.root_module.linkSystemLibrary("avahi-common", .{});
    libiio.root_module.linkSystemLibrary("aio", .{ .use_pkg_config = .no });

    configureCrossPaths(b, libiio.root_module);
    libiio.root_module.addIncludePath(b.path("src/Common"));
libiio.root_module.strip = true;

    libiio.root_module.addCSourceFiles(.{
        .files = &[_][]const u8{
            "src/iio/backend.c", "src/iio/buffer.c", "src/iio/channel.c", 
            "src/iio/context.c", "src/iio/device.c", "src/iio/dns_sd.c", 
            "src/iio/dns_sd_avahi.c", "src/iio/iiod-client.c", "src/iio/local.c", 
            "src/iio/lock.c", "src/iio/network.c", "src/iio/network-unix.c", 
            "src/iio/scan.c", "src/iio/sort.c", "src/iio/usb.c", 
            "src/iio/utilities.c", "src/iio/xml.cpp", "src/iio/deps/libini/libini.c",
            "src/iio/deps/libusb/core.c", "src/iio/deps/libusb/descriptor.c", 
            "src/iio/deps/libusb/hotplug.c", "src/iio/deps/libusb/io.c", 
            "src/iio/deps/libusb/strerror.c", "src/iio/deps/libusb/sync.c",
            "src/iio/deps/libusb/os/events_posix.c", "src/iio/deps/libusb/os/linux_netlink.c", 
            "src/iio/deps/libusb/os/linux_usbfs.c",
            "src/iio/deps/libusb/os/threads_posix.c", "src/main_linux.cpp", 
        },
        .flags = compile_flags,
    });
    b.installArtifact(libiio);

    // --- libad9361 ---
    const libad9361 = b.addLibrary(.{
        .name = "ad9361",
        .linkage = .dynamic,
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
        }),
    });

    libad9361.root_module.strip = true;
    libad9361.root_module.linkLibrary(libiio);
    libad9361.root_module.linkSystemLibrary("c++", .{});

    configureCrossPaths(b, libad9361.root_module);
    libad9361.root_module.addIncludePath(b.path("src/Common"));

    libad9361.root_module.addCSourceFiles(.{
        .files = &[_][]const u8{
            "src/libad9361/ad9361_baseband_auto_rate.c", "src/libad9361/ad9361_calculate_rf_clock_chain.c", 
            "src/libad9361/ad9361_design_taps.c", "src/libad9361/ad9361_fmcomms5_phase_sync.c", 
            "src/libad9361/ad9361_multichip_sync.c", "src/libad9361/filterdesigner/internal_design_filter_cg.c", 
            "src/libad9361/filterdesigner/internal_design_filter_cg_emxutil.c", 
            "src/libad9361/filterdesigner/rt_nonfinite.c", "src/libad9361/filterdesigner/rtGetInf.c", 
            "src/libad9361/filterdesigner/rtGetNaN.c", "src/main_linux.cpp",
        },
        .flags = compile_flags,
    });
    b.installArtifact(libad9361);

    // --- utils ---
    const utils = &[_][]const u8{ "iio_genxml", "iio_info", "iio_readdev", "iio_reg", "iio_writedev" };
    inline for (utils) |util_name| {
        const util_exe = b.addExecutable(.{
            .name = util_name,
            .root_module = b.createModule(.{
                .target = target,
                .optimize = optimize,
                .link_libc = true,
            }),
        });
        
        util_exe.root_module.strip = true;
        util_exe.root_module.linkLibrary(libiio);
        configureCrossPaths(b, util_exe.root_module);

        util_exe.root_module.addCSourceFiles(.{
            .files = &[_][]const u8{
                "src/utils/gen_code.c", "src/utils/iio_common.c",
                b.fmt("src/utils/{s}.c", .{util_name}),
            },
            .flags = compile_flags,
        });
        b.installArtifact(util_exe);
    }

    // --- iiod ---
    const iiod = b.addExecutable(.{
        .name = "iiod",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
        }),
    });
    
    iiod.root_module.strip = true;
    iiod.root_module.linkLibrary(libiio);
    iiod.root_module.linkSystemLibrary("aio", .{});
    iiod.root_module.linkSystemLibrary("avahi-client", .{});
    iiod.root_module.linkSystemLibrary("avahi-common", .{});
    iiod.root_module.addIncludePath(b.path("include"));
    iiod.root_module.addCSourceFiles(.{
        .files = &[_][]const u8{
            "src/iiod/dns-sd.c", "src/iiod/lexer.c", "src/iiod/ops.c", 
            "src/iiod/parser.c", "src/iiod/serial.c", "src/iiod/thread-pool.c", 
            "src/iiod/usbd.c", "src/iiod/iiod.c",
        },
        .flags = compile_flags,
    });
    b.installArtifact(iiod);
}

