FROM ghcr.io/lesomnus/dev-gcc:12

RUN sudo -u hypnos vcpkg install \
		catch2 \
	&& rm -rf \
		"${VCPKG_ROOT}/buildtrees" \
		"${VCPKG_ROOT}/packages" \
		"${VCPKG_DOWNLOADS}"/*



USER hypnos

RUN wget https://github.com/robbyrussell/oh-my-zsh/raw/master/tools/install.sh -O - | zsh || true \
	&& git clone https://github.com/zsh-users/zsh-autosuggestions         "${ZSH_CUSTOM:-~/.oh-my-zsh/custom}/plugins/zsh-autosuggestions"     \
	&& git clone https://github.com/zsh-users/zsh-syntax-highlighting.git "${ZSH_CUSTOM:-~/.oh-my-zsh/custom}/plugins/zsh-syntax-highlighting" \
	&& git clone https://github.com/zsh-users/zsh-completions             "${ZSH_CUSTOM:-~/.oh-my-zsh/custom}/plugins/zsh-completions"         \
	&& git clone https://github.com/supercrabtree/k                       "${ZSH_CUSTOM:-~/.oh-my-zsh/custom}/plugins/k"
COPY .zshrc /home/hypnos
ENV SHELL /bin/zsh
