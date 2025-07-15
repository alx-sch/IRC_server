#include <iostream>
#include "../include/defines.hpp"

int	main(int argc, char** argv)
{
	if (argc != 3)
	{
		std::cerr	<< YELLOW << "Usage: " << argv[0] << " <port> <password>\n" << RESET;
		return 1;
	}

	return 0;
}
