

Usage
=====


How to use with Singularity
---------------------------

In order to re-use the Docker image and run it, for example on the HPC with Singularity, first, create a personal access token with "read_registry" rights in the repo and then export this token in the terminal where the Singularity container should be run by the following


.. code:: bash

  export SINGULARITY_DOCKER_USERNAME=<username>
  export SINGULARITY_DOCKER_PASSWORD=<read_registry token>

  singularity run docker://ghcr.io/bedapub/ngs-tools:main make_cls -h


See also `Singularity documentation <https://sylabs.io/guides/3.1/user-guide/singularity_and_docker.html#making-use-of-private-images-from-private-registries>`_
