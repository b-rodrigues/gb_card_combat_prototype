{
  description = "Nix Game Boy LLM Development Kit";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs {
          inherit system;
        };

        gbdk =
          if system == "aarch64-linux" then
            let
              gbdkVersion = "4.3.0";
              sdccArm64 = pkgs.fetchzip {
                url = "https://github.com/gbdk-2020/gbdk-2020-sdcc/releases/download/sdcc-patched-gbdk-${gbdkVersion}/sdcc-14650-Linux-arm64.tar.gz";
                sha256 = "0xfkx6w679d0cnhnidb3q0rq4gqvs8v5nn2q7cp7hifq4hqx70ka";
              };
              gbdkSource = pkgs.fetchzip {
                url = "https://github.com/gbdk-2020/gbdk-2020/archive/refs/tags/${gbdkVersion}.tar.gz";
                sha256 = "0ygkxyqbjlmii9zdskzwk74hldwnymyla2n9glb2l4p33n2a3948";
              };
              gbdkPrecompiled = pkgs.fetchzip {
                url = "https://github.com/gbdk-2020/gbdk-2020/releases/download/${gbdkVersion}/gbdk-linux64.tar.gz";
                sha256 = "0slw2ag8ljgcb6v8qz35f3k3zm8y9nc0j451cgnval7q086ar5xp";
              };
              gbdkSupport =
                let
                  tools = [ "lcc" "ihxcheck" "bankpack" "png2asset"
                            "gbcompress" "makecom" "makebin"
                            "png2hicolorgb" "romusage" ];
                  buildList = builtins.concatStringsSep " " tools;
                in
                pkgs.stdenv.mkDerivation {
                  pname = "gbdk-support";
                  version = gbdkVersion;
                  src = gbdkSource;
                  buildPhase = ''
                    for d in ${buildList}; do
                      make -C gbdk-support/$d TARGETDIR=$out --no-print-directory
                    done
                  '';
                  installPhase = ''
                    mkdir -p $out/bin
                    for d in ${buildList}; do
                      cp gbdk-support/$d/$d $out/bin/
                    done
                  '';
                };
            in
            pkgs.stdenv.mkDerivation {
              pname = "gbdk";
              version = gbdkVersion;
              dontUnpack = true;
              nativeBuildInputs = [ pkgs.autoPatchelfHook ];
              buildInputs = [ pkgs.stdenv.cc.cc.lib pkgs.zlib ];
              installPhase = ''
                mkdir -p $out
                cp -r ${gbdkPrecompiled}/* $out/
                chmod -R u+w $out
                cp -f ${sdccArm64}/bin/* $out/bin/
                if [ -d "${sdccArm64}/libexec" ]; then
                  mkdir -p $out/libexec
                  cp -rf ${sdccArm64}/libexec/* $out/libexec/
                fi
                cp -f ${gbdkSupport}/bin/* $out/bin/
              '';
              meta = with pkgs.lib; {
                description = "GBDK-2020 Game Boy Development Kit";
                homepage = "https://github.com/gbdk-2020/gbdk-2020";
                license = licenses.mit;
                platforms = platforms.linux;
              };
            }
          else
            pkgs.stdenv.mkDerivation rec {
              pname = "gbdk";
              version = "4.3.0";

              src = pkgs.fetchzip {
                url = "https://github.com/gbdk-2020/gbdk-2020/releases/download/${version}/gbdk-linux64.tar.gz";
                sha256 = "0slw2ag8ljgcb6v8qz35f3k3zm8y9nc0j451cgnval7q086ar5xp";
              };

              nativeBuildInputs = [ pkgs.autoPatchelfHook ];
              buildInputs = [ pkgs.stdenv.cc.cc.lib pkgs.zlib ];

              installPhase = ''
                mkdir -p $out
                cp -r * $out/
              '';

              meta = with pkgs.lib; {
                description = "GBDK-2020 Game Boy Development Kit";
                homepage = "https://github.com/gbdk-2020/gbdk-2020";
                license = licenses.mit;
                platforms = platforms.linux;
              };
            };

        pyboyWheel =
          if system == "aarch64-linux" then {
            url = "https://files.pythonhosted.org/packages/ae/d7/adef77aa6ab916032e8dda5ec112daf42cf0c0767067caed5321f9b71069/pyboy-2.7.0-cp314-cp314-manylinux2014_aarch64.manylinux_2_17_aarch64.manylinux_2_28_aarch64.whl";
            hash = "sha256-T98YnkB1DqcybVGUwOxGuvNmUgftL8nxWO4+8FgA0Ho=";
          } else {
            url = "https://files.pythonhosted.org/packages/51/da/ce77683a235cbbf797c8eab25bd6dceabfd2c5109d75e06abbf7b27ff174/pyboy-2.7.0-cp314-cp314-manylinux2014_x86_64.manylinux_2_17_x86_64.manylinux_2_28_x86_64.whl";
            hash = "sha256-ivS1WtgnCzawDo0fnThu7Of3I1EYcuN+8twdR1bQnfc=";
          };

        pyboy = pkgs.python3.pkgs.buildPythonPackage rec {
          pname = "pyboy";
          version = "2.7.0";

          format = "wheel";

          src = pkgs.fetchurl {
            inherit (pyboyWheel) url hash;
          };

          nativeBuildInputs = [
            pkgs.autoPatchelfHook
          ];

          propagatedBuildInputs = with pkgs.python3.pkgs; [
            numpy
          ];

          pythonImportsCheck = [ "pyboy" ];

          # The wheel's METADATA declares pysdl2/pysdl2-dll, but both are
          # guarded by try/except ImportError in PyBoy and unnecessary for
          # headless (window="null") use; skip the runtime deps check.
          dontCheckRuntimeDeps = true;

          doCheck = false;

          meta = with pkgs.lib; {
            description = "Game Boy emulator written in Python";
            homepage = "https://github.com/Baekalfen/PyBoy";
            license = licenses.lgpl3Only;
            platforms = platforms.linux;
          };
        };

        hugetracker =
          if system == "aarch64-linux" then
            pkgs.stdenv.mkDerivation rec {
              pname = "hugetracker";
              version = "1.0.11";

              src = pkgs.fetchzip {
                url = "https://github.com/SuperDisk/hUGETracker/archive/refs/tags/v${version}.tar.gz";
                sha256 = "010qva5qqmrg3h5dmhmi4in2y2i1fas4197bjc0vjjx71szn1vln";
              };

              nativeBuildInputs = [ pkgs.fpc pkgs.lazarus ];

              buildPhase = ''
                HOME=$TMPDIR lazbuild --lazarusdir=${pkgs.lazarus}/share/lazarus --build-mode=Release src/uge2source/uge2source.lpi
              '';

              installPhase = ''
                mkdir -p $out/bin
                cp src/uge2source/uge2source $out/bin/uge2source
                ln -s ${pkgs.rgbds}/bin/rgbasm $out/bin/rgbasm-huge
              '';

              meta = with pkgs.lib; {
                description = "A music tracker for the Nintendo Game Boy (uge2source CLI)";
                homepage = "https://github.com/SuperDisk/hUGETracker";
                license = licenses.gpl3Only;
                platforms = platforms.linux;
              };
            }
          else
            pkgs.stdenv.mkDerivation rec {
              pname = "hugetracker";
              version = "1.0.11";

              src = pkgs.fetchzip {
                url = "https://github.com/SuperDisk/hUGETracker/releases/download/v${version}/hUGETracker-${version}-linux.zip";
                sha256 = "0nbgm80nwy78hz9hy3z181h39ahj4swgqcpx9317361pvvj58dl9";
                stripRoot = false;
              };

              nativeBuildInputs = [ pkgs.autoPatchelfHook ];
              buildInputs = with pkgs; [
                stdenv.cc.cc.lib
                fontconfig
                pango
                cairo
                atk
                gtk2-x11
                gdk-pixbuf
                glib
                libx11
                SDL2
              ];

              installPhase = ''
                mkdir -p $out/bin $out/share/hugetracker
                cp -r * $out/share/hugetracker/
                ln -s $out/share/hugetracker/hUGETracker $out/bin/hUGETracker
                ln -s $out/share/hugetracker/hUGETracker $out/bin/hugetracker
                ln -s $out/share/hugetracker/uge2source $out/bin/uge2source
                ln -s $out/share/hugetracker/rgbasm $out/bin/rgbasm-huge
              '';

              meta = with pkgs.lib; {
                description = "A music tracker for the Nintendo Game Boy";
                homepage = "https://github.com/SuperDisk/hUGETracker";
                license = licenses.gpl3Only;
                platforms = platforms.linux;
              };
            };
      in
      {
        packages.gbdk = gbdk;
        packages.hugetracker = hugetracker;

        devShells.default = pkgs.mkShell {
          name = "gb-dev-shell";

          GBDKDIR = "${gbdk}/";
          GBDK_HOME = "${gbdk}/";

          buildInputs = [
            gbdk
            hugetracker
            pkgs.rgbds
            pkgs.sameboy
            pkgs.mgba
            pkgs.gnumake
            pkgs.git
            pkgs.xvfb-run
            pkgs.imagemagick
            pkgs.python3
            pkgs.python3Packages.pillow
            pyboy
            pkgs.nodejs
          ];

          shellHook = ''
            export GBDKDIR="${gbdk}/"
            export GBDK_HOME="${gbdk}/"
          '';
        };
      }
    );
}
