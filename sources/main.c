/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jsiller <jsiller@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/10/25 22:36:12 by jsiller           #+#    #+#             */
/*   Updated: 2021/11/05 16:35:14 by jsiller          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <minishell.h>
#include <parser.h>
#include <utilities.h>
#include <execute.h>
#include <signals.h>
#include <env.h>

void	exit_free(t_cmds *cmd, char *str, bool status)
{
	static t_cmds	*data;
	static char		*s;

	if (status == 0)
	{
		data = cmd;
		s = str;
	}
	else
	{
		clear_history();
		clear_list(data, 0);
		free(s);
	}
}

static int	handlearg(char *argv[])
{
	t_cmds	*cmds;

	cmds = NULL;
	if (append_list(&cmds))
		return (1);
	cmds = parse(argv[1], cmds, argv[0]);
	if (!cmds)
		return (1);
	if (find_last(cmds)->cmd == NULL)
		cmds = delete_node(find_last(cmds));
	cmds = find_listhead(cmds);
	return (clear_list(cmds, ((int)execute(cmds))));
}

static int	handleinput(char *str, char *argv)
{
	t_cmds	*cmds;
	char	*symbol;

	symbol = check_input(str, 0);
	if (!symbol)
	{
		cmds = NULL;
		if (append_list(&cmds))
			return (1);
		cmds = parse(str, cmds, argv);
		if (!cmds)
			return (1);
		if (find_last(cmds)->cmd == NULL)
			cmds = delete_node(find_last(cmds));
		cmds = find_listhead(cmds);
		exit_free(cmds, str, 0);
		g_ourenv.exit_status = execute(cmds);
		clear_list(cmds, 0);
		return (0);
	}
	ft_putstr_fd("minishell: syntax error near unexpected token `", 2);
	ft_putstr_fd(symbol, 2);
	ft_putstr_fd("\'\n", 2);
	free(symbol);
	return (0);
}

int	main(int argc, char *argv[], char **env)
{
	char	*str;

	if (read_env(env))
		ft_putstr_fd("minishell: env: error trying to create the env\n", 2);
	signal(SIGQUIT, SIG_IGN);
	if (argc == 2)
		return (handlearg(argv));
	while (1)
	{
		signal(SIGINT, SIG_IGN);
		signal(SIGINT, gsignal_ctlc);
		changetermios(false);
		str = readline("minishell-2.0$ \x1b[s");
		if (str == NULL && write(1, "\x1b[uexit\n", 9) && !changetermios(true))
			exit(1);
		if (handleinput(str, argv[0]))
			ft_putstr_fd("minishell: malloc error during runtime\n", 2);
		if (str && str[0])
			add_history(str);
		free(str);
	}
	return (0);
}
