FROM ghcr.io/lesomnus/dev-gcc:12



USER hypnos

RUN wget https://github.com/robbyrussell/oh-my-zsh/raw/master/tools/install.sh -O - | zsh || true \
	&& git clone https://github.com/zsh-users/zsh-autosuggestions         "${ZSH_CUSTOM:-${HOME}/.oh-my-zsh/custom}/plugins/zsh-autosuggestions"     \
	&& git clone https://github.com/zsh-users/zsh-syntax-highlighting.git "${ZSH_CUSTOM:-${HOME}/.oh-my-zsh/custom}/plugins/zsh-syntax-highlighting" \
	&& git clone https://github.com/zsh-users/zsh-completions             "${ZSH_CUSTOM:-${HOME}/.oh-my-zsh/custom}/plugins/zsh-completions"         \
	&& git clone https://github.com/supercrabtree/k                       "${ZSH_CUSTOM:-${HOME}/.oh-my-zsh/custom}/plugins/k"
COPY .zshrc /home/hypnos
ENV SHELL /bin/zsh
