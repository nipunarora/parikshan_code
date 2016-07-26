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
COPY install-mysql.sh ~/install-mysql.sh
RUN cd ~ && sh install-mysql.sh


EXPOSE 22
#ENTRYPOINT [ "service", "ssh", "start"]
ENTRYPOINT service ssh restart && ~/mysql/bin/mysqld_safe --user=mysql --skip-grant-tables &
