{
  description = "Flake shell";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    flake-parts.url = "github:hercules-ci/flake-parts";
  };

  outputs =
    inputs@{ flake-parts, nixpkgs, ... }:
    flake-parts.lib.mkFlake { inherit inputs; } {
      systems = nixpkgs.lib.platforms.all;

      perSystem =
        { pkgs, ... }:
        {
          devShells.default = pkgs.mkShell {
            packages = [
              pkgs.libgpiod_1
              pkgs.cmake
              pkgs.ninja
              pkgs.llvmPackages_20.clang-tools
              pkgs.llvmPackages_20.libcxxClang
              pkgs.cmake-format
              pkgs.cmake-language-server
            ];
            shellHook = ''
              export CC="${pkgs.llvmPackages_20.libcxxClang}/bin/clang"
              export CXX="${pkgs.llvmPackages_20.libcxxClang}/bin/clang++"
            '';

          };

          packages.default = pkgs.llvmPackages_20.libcxxStdenv.mkDerivation {
            name = "demo";
            src = ./.;
            buildInputs = [
              pkgs.libgpiod_1
              pkgs.cmake
              pkgs.ninja
            ];

          };
        };
    };
}
