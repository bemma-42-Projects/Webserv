/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: julien <julien@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/20 10:32:37 by julien            #+#    #+#             */
/*   Updated: 2026/04/20 15:44:16 by julien           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "utils.hpp"

// rend un file descriptor non bloquant
// permet de s'assurer que les appels recv et send ne bloquent jamais la boucle epoll
void    setNonBlocking(int fd) {
    int flags;

    flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		throw std::runtime_error("fcntl(F_GETFL) failed");
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
		throw std::runtime_error("fcntl(F_SETFL) failed");
}
