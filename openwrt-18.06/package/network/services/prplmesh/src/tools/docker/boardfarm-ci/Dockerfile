FROM python:3.8-slim-buster

# Debian dependencies to install docker and docker compose from docker.com
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y \
       apt-transport-https \
       ca-certificates \
       curl \
       gcc \
       gnupg-agent \
       jq \
       libsnmp-dev \
       software-properties-common \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Install docker and docker-compose from docker.com repositories
RUN curl -fsSL https://download.docker.com/linux/debian/gpg | apt-key add -
RUN add-apt-repository "deb [arch=$(dpkg --print-architecture)] https://download.docker.com/linux/debian $(lsb_release -cs) stable"
RUN apt-get update \
    && apt-get install -y \
       containerd.io \
       docker-ce \
       docker-ce-cli \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

RUN curl -L "https://github.com/docker/compose/releases/download/1.26.0/docker-compose-$(uname -s)-$(uname -m)" -o /usr/local/bin/docker-compose
RUN chmod 755 /usr/local/bin/docker-compose

# Installation of boardfarm
RUN git clone https://github.com/mattsm/boardfarm.git \
    && cd boardfarm \
    && git checkout 100521fde1fb67536682cafecc2f91a6e2e8a6f8 \
    && python3 setup.py install

# Installation of the python packages needed by the tests
COPY requirements.txt /app/requirements.txt
WORKDIR app
RUN pip3 install -r requirements.txt

# Installation of Debian dependencies needed by the tests
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y \
       iperf3 \
       iproute2 \
       iw \
       net-tools \
       tshark \
       wireshark-common \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

