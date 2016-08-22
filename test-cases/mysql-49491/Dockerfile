############################################################
# dockerfile from nipun
############################################################
# Set the base image to Ubuntu
FROM parikshan/baseimage

# File Author / Maintainer
MAINTAINER nipun

ENV HOME /root
ENV DEBIAN_FRONTEND noninteractive

RUN apt-get install -y wget
RUN apt-get install -y libncurses-dev
COPY Makefile /root/.
COPY install-mysql.sh /root/install-mysql.sh
RUN cd /root/ && sh install-mysql.sh


EXPOSE 22
#ENTRYPOINT [ "service", "ssh", "start"]
#ENTRYPOINT service ssh restart && /root/mysql/bin/mysqld_safe --user=mysql --skip-grant-tables 
ENTRYPOINT service ssh restart && /bin/bash 
