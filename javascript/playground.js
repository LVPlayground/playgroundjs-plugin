// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

require('entities/player.js');
require('libraries/timers.js');

self.addEventListener('playerconnect', event => {
  let player = new Player(event.playerid);
  player.name = 'YourNewName';
  
  wait(1500).then(() =>
      console.log('Hello, ' + player.name + '!'));
});
