{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-22.11";
    future.url = "github:NixOS/nixpkgs/nixos-unstable";
    outdated.url = "github:NixOS/nixpkgs/nixos-21.05";
  };
  outputs =
    { self, nixpkgs, future, outdated, flake-utils, bundlers }:
    let

      version = "1.2.0";
      base_libs = pkgs:
        with pkgs; [
          cli11
          cmake
          fmt_8
          fmt_8.dev
          gsl
          hdf5-cpp.dev
          ninja
          zlib
        ];

    in
    flake-utils.lib.eachSystem [ "x86_64-linux" "aarch64-linux" ] (system:


    let
      pkgs = import nixpkgs { inherit system; };
      next = import future { inherit system; };
    in
    {
      devShells.default = pkgs.stdenv.mkDerivation {
        name = "np-shell";
        buildInputs = base_libs pkgs
          ++ (with pkgs; [
          (pkgs.clang-tools.override {
            llvmPackages = pkgs.llvmPackages_13;
          })
          ccache
          ccls
          cmake-format
          cmake-language-server
          distcc
          gdb
          valgrind
        ]);
        CMAKE_CXX_COMPILER_LAUNCHER = "${pkgs.ccache}/bin/ccache";
        CMAKE_CXX_FLAGS_DEBUG = "-g -O0";
        CXXL = "${pkgs.stdenv.cc.cc.lib}";
      };

      apps = {
        default =
          flake-utils.lib.mkApp { drv = self.packages.${system}.np; };
      };

      packages = {
        np = pkgs.stdenv.mkDerivation ({
          inherit version;
          pname = "np";
          src = builtins.path {
            path = ./.;
            name = "np-src";
          };
          buildInputs = base_libs pkgs;
          nativeBuildInputs = [ pkgs.wrapGAppsHook ];

          cmakeFlags = [ "-G Ninja -DCONAN:bool=Off" ];
          installPhase = ''
            mkdir -p $out/bin
            mv np $out/bin/
          '';

          meta = with pkgs.lib; {
            description = "Processor for NeXuS files";
            homepage = "https://github.com/disorderedmaterials/np";
            license = licenses.gpl3;
            maintainers = with maintainers; [ rprospero ];
          };
        });

        singularity =
          nixpkgs.legacyPackages.${system}.singularity-tools.buildImage {
            name = "np-${version}";
            diskSize = 1024 * 50;
            memSize = 1024 * 2;
            contents = [
              self.packages.${system}.np
            ];
            runScript = "${self.packages.${system}.np}/bin/np $@";
          };
      };
    });
}
