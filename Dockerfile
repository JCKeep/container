FROM ubuntu_latest

RUN /usr/bin/apt update

RUN /usr/bin/apt install -y nginx

