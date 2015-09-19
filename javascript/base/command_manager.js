// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

class Command {
  constructor(player, commandText) {
    this.player = player;
    this.arguments = commandText.split(' ');
  }

  isValid() {
    return this.arguments.length && this.arguments[0].startsWith('/');
  }

  get command() {
    return this.arguments[0].substr(1);
  }

  toString() {

  }
}

class CommandManager {
  constructor() {
    this.commands = {};
  }

  registerCommand(name, fn) {
    if (this.commands.hasOwnProperty(name))
      throw new Error('A command named ' + name + ' has already been registered.');

    this.commands[name] = fn;
  }

  triggerCommand(player, commandText) {
    let command = new Command(player, commandText);
    if (!command.isValid())
      return false;

    if (!this.commands.hasOwnProperty(command.command))
      return false;

    this.commands[command.command].call(null, command);
    return true;
  }
};

//
self.addEventListener('playercommandtext', event => {
  if (global.commands.triggerCommand(new Player(event.playerid),
                                     event.commandtext)) {
    event.preventDefault();
  }
});

// Expose an instance of the command manager on the global scope.
global.commands = new CommandManager();
