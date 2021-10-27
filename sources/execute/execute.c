/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jsiller <jsiller@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/10/27 00:13:31 by jsiller           #+#    #+#             */
/*   Updated: 2021/10/27 22:28:13 by jsiller          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <minishell.h>
#include <execute.h>
#include <signals.h>
#include <builtins.h>

extern char **environ;

static const t_builtins	built_cmd[] = {
	{"echo", &bt_echo},
	{"cd", &bt_cd},
	{"pwd", &bt_pwd},
	{"export", &bt_export},
	{"unset", &bt_unset},
	{"env", &bt_env},
	{"exit", &bt_exit},
	{NULL, NULL}
};

static void	check_builtin(char **cmd)
{
	int	i;

	i = 0;
	while (built_cmd[i].name)
	{
		if (!ft_strncmp(cmd[0], built_cmd[i].name, ft_strlen(built_cmd[i].name) + 1))
		{
			exit(built_cmd[i].func(cmd));
		}
		i++;
	}
}

static void	child(t_execute *exec, t_cmds *data)
{
	signal(SIGINT, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
	changetermios(true);
	char	*str;
	if (data->read == 1 && dup2(exec->s_fd, 0) == -1)
	{
		ft_putstr_fd("Dup error\n", 2);
		return ;
	}
	if (data->write == 1 && dup2(exec->fd[1], 1) == -1)
	{
		ft_putstr_fd("Dup error\n", 2);
		return ;
	}
	if (data->write == 0 && dup2(exec->s_out, 1) == -1)
	{
		ft_putstr_fd("Dup error\n", 2);
		return ;
	}
	if (data->read == 0 && dup2(exec->s_in, 0) == -1)
	{
		ft_putstr_fd("Dup error\n", 2);
		return ;
	}
	close(exec->fd[0]);
	close(exec->fd[1]);
	close(exec->s_fd);
	check_builtin(data->cmd);
	find_command(data->cmd[0], &str, environ);
	ft_lstclear(&(exec->lst), free);
	execve(str, data->cmd, environ);
	ft_putstr_fd("minishell: ", 2);
	ft_putstr_fd(str, 2);
	ft_putendl_fd(": cmd not found", 2);
	exit(127);
}

void	ft_wait(void *pid)
{
	waitpid(((t_pid *)pid)->pid, &((t_pid *)pid)->exit, 0);
	if (WIFEXITED(((t_pid *)pid)->exit))
		((t_pid *)pid)->exit = WEXITSTATUS(((t_pid *)pid)->exit);
}

static void	parent(t_execute *exec, t_cmds *data)
{
	if (!ft_strnstr(data->cmd[0], "/minishell", 12))
		signal(SIGINT, fsignal_ctlc);
	else
		signal(SIGINT, SIG_IGN);
	if (exec->s_fd != -1)
		close(exec->s_fd);
	close(exec->fd[1]);
	exec->s_fd = dup(exec->fd[0]);
	close(exec->fd[0]);
	if (data->operators != OPERATORS_NONE)
	{
		ft_lstiter(exec->lst, ft_wait);
		exec->exit = ((t_pid *)ft_lstlast(exec->lst)->content)->exit;
	}
}

/*
** execute gets a list of arguements and executes them
** scope always starts with 0 and only gets increased 
** incase of an recursive call
*/
unsigned char	execute(t_cmds *data)
{
	t_pid			*pid;
	t_list			*tmp;
	t_execute		exec;
							if (!data)
							{
								ft_putstr_fd("Got a null pointer, something went wrong\n", 2);
								return (1);
							}
	exec.s_fd = -1;
	exec.lst = 0;
	exec.s_in = dup(0);
	exec.s_out = dup(1);
	exec.exit = 0;
	while (data != 0)
	{
		pid = malloc(sizeof(*pid));
							if (!pid)
							{
								ft_putstr_fd("Malloc error\n", 2);
								ft_lstclear(&(exec.lst), free);
								return (1);
							}
		if (pipe(exec.fd) == -1)
							{
								ft_putstr_fd("Pipe error\n", 2);
								ft_lstclear(&(exec.lst), free);
								return (1);
							}
		pid ->pid = fork();
							if (pid->pid == -1)
							{
								ft_putstr_fd("Fork error\n", 2);
								ft_lstclear(&(exec.lst), free);
								close(exec.fd[0]);
								close(exec.fd[1]);
								return (1);
							}
		if (pid->pid == 0)
		{
			child(&exec, data);
		}
		else 
		{
			tmp = ft_lstnew(pid);
								if (!tmp)
								{
									ft_putstr_fd("Error\n", 2);
									return (1);
								}
			ft_lstadd_back(&(exec.lst), tmp);
			parent(&exec, data);
		}
		data = data->next;
		if (data && data->previous->operators == OPERATORS_AND && exec.exit != 0)
		{
			data = data->next;
			while (data && (data->previous->operators != OPERATORS_OR))
				data = data->next;
		}
		while (data && data->previous->operators == OPERATORS_OR && exec.exit == 0)
		{
			data = data->next;
			while (data && (data->previous->operators != OPERATORS_AND))
				data = data->next;
		}
	}
	close(exec.s_fd);
	ft_lstiter(exec.lst, ft_wait);
	exec.exit = ((t_pid *)ft_lstlast(exec.lst)->content)->exit;
	ft_lstclear(&exec.lst, free);
	return (exec.exit);
}
