# Pull base image
FROM fedora:28

# Install basic tools
RUN dnf update -y \
 && dnf install -y \
 	asciidoc \
	autoconf \
	automake \
	bash-completion \
	bc \
	clang \
	cmake \
	file \
	findutils \
	fuse \
	fuse-devel \
	gcc \
	gcc-c++ \
	gdb \
	git \
	glib2-devel \
	hub \
	json-c-devel \
	kmod-devel \
	lbzip2 \
	libtool \
	libudev-devel \
	libunwind-devel \
	libuuid-devel \
	libuv-devel \
	make \
	man \
	ncurses-devel \
	openssh-server \
	pandoc \
	passwd \
	pkgconfig \
	rpm-build \
	rpm-build-libs \
	rpmdevtools \
	sudo \
	tar \
	wget \
	which \
	xmlto \
	autoconf \
	automake \
	clang \
	cmake \
	doxygen \
	gcc \
	gdb \
	git \
	hub \
	libunwind-devel \
	make \
	man \
	ncurses-devel \
	open-sans-fonts \
	passwd \
	perl-Text-Diff \
	rpm-build \
	rpm-build-libs \
	rpmdevtools \
	SFML-devel \
	sudo \
	tar \
	wget \
	which \
	json-c-devel \
	kmod-devel \
	libudev-devel \
	asciidoc \
	xmlto \
	libuuid-devel \
	libtool \
	bash-completion \
	ndctl-devel \
	daxctl-devel \
	vim \
	clang \
	libaio-devel \
	boost-devel \ 
 && dnf clean all

# Add user
ENV USER user
ENV USERPASS pass
RUN useradd -m $USER
RUN echo $USERPASS | passwd $USER --stdin
RUN gpasswd wheel -a $USER
USER $USER

# Set required environment variables
ENV OS fedora
ENV OS_VER 28
ENV PACKAGE_MANAGER rpm
ENV NOTTY 1
USER root
RUN git clone https://github.com/pmem/pmdk
RUN cd pmdk
WORKDIR pmdk
RUN make
RUN make install prefix=/usr/
RUN cd /
WORKDIR /
RUN git clone https://github.com/pmem/libpmemobj-cpp.git
RUN cd libpmemobj-cpp
RUN mkdir build
RUN cd build
WORKDIR /libpmemobj-cpp/build
RUN cmake ..
RUN make
RUN make install
RUN cd /
WORKDIR /
RUN git clone https://github.com/google/googletest.git
RUN cd googletest
WORKDIR googletest
RUN cmake CMakeLists.txt
RUN make
RUN make install
RUN cd /
WORKDIR /
RUN ldconfig
RUN git clone https://github.com/kamciokodzi/distributed-nvm-hashtable
WORKDIR /distributed-nvm-hashtable/map
RUN g++ main.cpp -o main -std=c++17 -lpmem -lpmemobj -lpthread
WORKDIR /
RUN mkdir /mnt/ramdisk
# RUN sudo mount -t tmpfs -o size=512m tmpfs /mnt/ramdisk

# NVMHashMap tests
RUN dnf install -y gtest-devel
RUN cd /distributed-nvm-hashtable/map/tests
WORKDIR /distributed-nvm-hashtable/map/tests
RUN cmake CMakeLists.txt
