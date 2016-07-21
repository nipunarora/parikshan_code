############################################################
# dockerfile from nipun
############################################################
# Set the base image to Ubuntu
FROM phusion/baseimage:0.9.15

# File Author / Maintainer
MAINTAINER tsar2512

ENV HOME /root
ENV DEBIAN_FRONTEND noninteractive

RUN apt-get update
RUN apt-get install -y vim
RUN apt-get install -y emacs23-nox
RUN apt-get install -y screen
RUN apt-get install -y git
RUN apt-get install -y build-essential
RUN apt-get install -y openssh-server
RUN apt-get install -y curl
RUN apt-get install -y dtach

#COPY vimrc /root/.vimrc
COPY screenrc /root/.screenrc

#CMD ssh-keygen -q -t rsa -N '' -f /root/.ssh/id_rsa
#COPY authorized_keys /root/.ssh/authorized_keys

EXPOSE 22
#ENTRYPOINT [ "service", "ssh", "start"]
ENTRYPOINT service ssh restart && /bin/bash
