FROM ghcr.io/bedapub/bios-to-go:main

LABEL author="Roland Schmucki" \
      description="Helper tools for ngs data analysis" \
      maintainer="roland.schmucki@roche.com"

WORKDIR /usr/ngs-tools/
COPY . /usr/ngs-tools/

RUN mkdir bin && make

ENV PATH "$PATH:/usr/ngs-tools/bin"
