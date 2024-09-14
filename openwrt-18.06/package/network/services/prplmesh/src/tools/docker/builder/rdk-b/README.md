== RDK-B Build ==

This directory builds for RDK-B in a Docker container.
The scripts optimise the build time by making use of the download and sstate caches of OpenEmbedded.

* Export the RDK credentials in the environment.
  The username must be set in RDK_GIT_USER, the password in RDK_GIT_TOKEN.
  Alternatively, the password can be written base64-encoded to a file, and the filename is put in the RDK_GIT_TOKEN_FILE environment variable.
* Run `tools/docker/builder/rdk-b/build.sh -d <device>` to start the build.
  The `-d` option is mandatory.
  Supported devices is currently only `turris-omnia`.
* By default, the download and sstate cache are put in the `rdk` subdirectory of the top-level directory.
  An alternative location can be given with the `--cache` option of the top-level directory.
* The build results will be made available in the `build/<device>` directory of the top-level directory.
  There will be an ipk file for prplmesh, and one or more image files for the full system.

The same docker image is used for all devices, but a different build script is used.

The prplmesh recipe is taken from the [meta-prplmesh](https://gitlab.com/prpl-foundation/prplmesh/meta-prplmesh/) repository.
However, the source is overridden to use the current directory instead of what is checked in in git.

The RDK-B sources are part of the docker image.
Unfortunately, RDK-B uses `repo` with branch names to track the different repositories.
Therefore, it is not possible to specify exactly which version will be used.
Thus, the RDK-B sources will be different when the image is created at another time.
