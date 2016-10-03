############################################################
# dockerfile from nipun
############################################################
# Set the base image to Ubuntu
FROM parikshan/baseimage

# File Author / Maintainer
MAINTAINER nipun

ENV HOME /root
ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update
RUN apt-get -y upgrade

#RUN apt-get install -y libssl-dev zlib1g-dev g++ autoconf automake wget libncurses-dev wget maven openjdk-6-jdk findbugs
RUN useradd -ms /bin/bash hadoop
RUN echo "hadoop:hadoop"|chpasswd

COPY * /root/

#COPY bashrc /root/.bashrc
#COPY core-site.xml /root/.
#COPY hdfs-site.xml /root/.
#COPY mapred-site.xml /root/.
#COPY hadoop-env.sh /root/.
#COPY install-hadoop.sh /root/.
#RUN cd /root/ && sh install-hadoop.sh

EXPOSE 22
#ENTRYPOINT [ "service", "ssh", "start"]
#ENTRYPOINT service ssh restart && /root/mysql/bin/mysqld_safe --user=mysql --skip-grant-tables 
ENTRYPOINT service ssh restart && /bin/bash 
