############################################################
# dockerfile from nipun
############################################################
# Set the base image to Ubuntu
FROM parikshan/baseimage

# File Author / Maintainer
MAINTAINER nipun

ENV HOME /root
ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update && apt-get -y upgrade

COPY install.sh /root/install.sh
#RUN cd /root/ && git clone https://github.com/antirez/redis.git && cd redis && git checkout bfc197c3b604baf0dba739ea174d5054284133f0 && make
RUN cd /root/ && sh install.sh

EXPOSE 22
#ENTRYPOINT [ "service", "ssh", "start"]
ENTRYPOINT service ssh restart && /bin/bash
