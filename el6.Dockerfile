FROM centos:6
RUN yum groupinstall -y "Development tools" && yum clean all
RUN yum install -y rpmdevtools yum-utils && yum clean all
RUN yum install -y epel-release && yum clean all
COPY mcu-el6.repo /etc/yum.repos.d/

ARG USERID=1000
ARG GROUPID=1000
RUN groupadd -g ${USERID} rpmbuild && useradd -u ${USERID} -g ${GROUPID} rpmbuild
WORKDIR /home/rpmbuild

# Install dependencies referenced by the spec file, but cache the installed RPMs so they can be fingerprinted later
ARG SPECFILE
COPY ${SPECFILE} .
# Invalidate Docker cache if yum repo metadata has changed
ADD http://nexus.bbcollab.net/yum/mcu-develop/repodata/repomd.xml /tmp/
RUN yum-builddep -y --downloadonly --downloaddir=/tmp/build-deps ${SPECFILE} && \
    ([ -z "$(ls -A /tmp/build-deps/*.rpm)" ] || yum install -y /tmp/build-deps/*.rpm) && \
    yum clean all

USER rpmbuild