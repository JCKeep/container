FROM ubuntu_latest

RUN /usr/bin/apt update

COPY main.c /root/main.c

RUN /usr/bin/apt install -y redis

