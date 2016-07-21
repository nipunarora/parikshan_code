############################################################
# dockerfile from nipun
############################################################
# Set the base image to Ubuntu
FROM parikshan/baseimage

# File Author / Maintainer
MAINTAINER nipun

ENV HOME /root
ENV DEBIAN_FRONTEND noninteractive

COPY install.sh /root/install.sh
COPY ivy.xml /root/ivy.xml
COPY ivysettings.xml /root/ivysettings.xml

RUN apt-get install -y openjdk-7-jdk
RUN apt-get install -y ant
RUN cd /root/ && sh install.sh

EXPOSE 22
#ENTRYPOINT [ "service", "ssh", "start"]
ENTRYPOINT service ssh restart && /bin/bash
