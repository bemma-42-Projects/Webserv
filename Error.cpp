#include "Error.hpp"

int Error::error_ = 0;

void	Error::setError(int error)
{
	error_ = error;
}