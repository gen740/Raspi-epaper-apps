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
        let
          bcm2835 = pkgs.callPackage ./external/bcm2835.nix { };
          stb_image = pkgs.callPackage ./external/stb_image.nix { };
        in
        {
          devShells.default = pkgs.mkShell {
            packages =
              [
                pkgs.python312Packages.venvShellHook
                pkgs.cmake
                pkgs.ninja
                pkgs.pkg-config
                pkgs.llvmPackages_20.clang-tools
                pkgs.llvmPackages_20.libcxxClang
                pkgs.cmake-format
                pkgs.cmake-language-server
                pkgs.protobuf
                pkgs.openssl
                pkgs.grpc
                stb_image
              ]
              ++ (
                if pkgs.stdenv.isLinux then
                  [
                    pkgs.libgpiod
                    bcm2835
                  ]
                else
                  [ ]
              );
            venvDir = "venv";
          };

          packages = {
            default = pkgs.stdenv.mkDerivation {
              name = "raspberry-pi-apps";
              src = ./.;
              nativeBuildInputs = [
                pkgs.cmake
                pkgs.ninja
                pkgs.pkg-config
              ];
              buildInputs = [
                pkgs.libgpiod
                bcm2835
                pkgs.protobuf
                pkgs.grpc
                pkgs.openssl
                stb_image
              ];

            };
          };
        };
    };
}
