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
RUN apt-get install \
ruby
RUN gem install bundler && bundle install

COPY install.sh /root/install.sh
RUN cd /root/ && sh install.sh
COPY * /root/

EXPOSE 22
#ENTRYPOINT [ "service", "ssh", "start"]
ENTRYPOINT service ssh restart && /bin/bash
