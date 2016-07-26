############################################################
# dockerfile from nipun
############################################################
# Set the base image to Ubuntu
FROM parikshan/baseimage

# File Author / Maintainer
MAINTAINER nipun

ENV HOME /root
ENV DEBIAN_FRONTEND noninteractive

RUN apt-get install -y openjdk-7-jdk
RUN apt-get install -y wget

COPY bashrc /root/.bashrc
COPY core-site.xml /root/.
COPY hdfs-site.xml /root/.
COPY mapred-site.xml /root/.
COPY hadoop-env.sh /root/.
COPY install-hadoop.sh /root/.

RUN sh /root/install-hadoop.sh

EXPOSE 22
#ENTRYPOINT [ "service", "ssh", "start"]
ENTRYPOINT service ssh restart && /bin/bash
