# include "main.h"

/**
 * print_prompt - Display the shell prompt and read user input.
 *
 * Return: Pointer to the input buffer.
 */

char *print_prompt(void)
{
	ssize_t prompt;
	char *buffer = NULL, *cwd = NULL, *user = getlogin();
	char host[1024];
	size_t buffsize = 0;

	if (gethostname(host, sizeof(host)) != 0)
		return (NULL);

	cwd = getcwd(NULL, 0);
	if (cwd == NULL)
		return (NULL);

	if (isatty(STDIN_FILENO) == 1)
		fprintf(stdout, "%s@%s:%s$ ", user, host, cwd);

	prompt = getline(&buffer, &buffsize, stdin);

	if (prompt == -1)
	{
		if (buffer)
			free(buffer);
		free(cwd);
		free(user);
	}
	if (buffer[prompt - 1] == '\n')
	{
		buffer[prompt - 1] = '\0';
		prompt--;
	}

	free(cwd);
	free(user);
	return (buffer);
}

/**
 * parser - Tokenize a string based on a separator.
 * @str: The string to be tokenized.
 * @separator: The separator used for tokenization.
 *
 * Return: An array of tokens.
 */

char **parser(char *str, char *separator)
{
	char **tokens = NULL, *token = NULL, *aux = NULL;
	int i, count = 0, j;

	if (!str)
		return (NULL);
	for (i = 0; str[i]; i++)
	{
		for (j = 0; separator[j]; j++)
		{
			if (str[i] == separator[j] || str[i + 1] == '\0')
				count++;
		}
	}
	count++;
	tokens = malloc(count * sizeof(char *));
	if (!tokens)
	{
		perror("Unable to malloc space\n");
		return (NULL);
	}
	aux = strdup(str);
	token = strtok(aux, separator);
	for (i = 0; token; i++)
	{
		tokens[i] = strdup(token);
		if (!tokens[i])
		{
			perror("Unable to malloc space\n");
			free_all(tokens, aux);
			return (NULL);
		}
		token = strtok(NULL, separator);
	}
	tokens[i] = NULL;
	free(aux);
	return (tokens);
}


/**
 * search_cmd - Search for a command in the directories specified by PATH.
 * @cmd: The command to search for.
 *
 * Return: The full path to the command if found, otherwise NULL.
 */

char *search_cmd(char *cmd)
{
	char *full_path = _getenv("PATH=");
	char *path = NULL;
	char **directories = parser(full_path, ":");
	int i;

	if (cmd && full_path && directories)
	{
		if (access(cmd, F_OK) == 0)
		{
			free_all(directories, full_path);
			return (strdup(cmd));
		}

		for (i = 0; directories[i]; i++)
		{
			path = calloc(strlen(directories[i]) + strlen(cmd) + 2, sizeof(char));
			if (path)
			{
				strcat(path, directories[i]), strcat(path, "/"), strcat(path, cmd);
				if (access(path, F_OK) == 0)
				{
					free_all(directories, full_path);
					return (path);
				}
				free(path);
			}
			else
			{
				perror("Unable to malloc space\n");
				free_all(directories, full_path);
				return (NULL);
			}
		}
		free_all(directories, full_path);
	}
	return (NULL);
}


/**
 * create_process - Create and execute a new process for a given command.
 * @buff: An array of strings containing the command and its arguments.
 * @count: The command count for error reporting.
 * @shell: the name of the shell
 */

int create_process(char *shell, char **buff, int count)
{
	int status = 0;
	char *full_path = NULL;
	struct stat path_stat;
	pid_t pid;

	if (stat(buff[0], &path_stat) == 0 && (path_stat.st_mode & S_IXUSR))
	{
		pid = fork();
		if (pid == -1)
		{
			perror("Can't fork");
			return (-1);
		}
		else if (pid == 0)
		{
			execve(buff[0], buff, environ);
			perror("Error in execve");
			free(buff);
		}
		waitpid(pid, &status, 0);
	}
	else
	{
		full_path = search_cmd(buff[0]);
		if (full_path)
		{
			if (access(full_path, X_OK) != 0)
			{
				fprintf(stderr, "%s: %s: Permission denied\n", shell, full_path);
				free(full_path);
			}
			else
				status = execute_with_path(buff, full_path);
		}
		else
			fprintf(stderr, "%s: %s: %d: not found\n", shell, buff[0], count);
	}
	return (status);
}

/**
 * execute_with_path - Execute a command using the full path.
 * @buff: An array of strings containing the command and its arguments.
 * @full_path: The full path to the command.
 */
int execute_with_path(char **buff, char *full_path)
{
	int status = 0;
	pid_t pid = fork();

	if (pid == -1)
	{
		perror("Can't fork");
		return (-1);
	}
	else if (pid == 0)
	{
		execve(full_path, buff, environ);
		perror("Error in execve");
		free_all(buff, full_path);
	}
	waitpid(pid, &status, 0);
	free(full_path);
	return (0);
}
