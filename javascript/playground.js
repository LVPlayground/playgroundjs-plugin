// Copyright 2015 Las Venturas Playground. All rights reserved.
// Use of this source code is governed by the MIT license, a copy of which can
// be found in the LICENSE file.

function GetPlayerName(playerid) {
  return pawnInvoke('GetPlayerName', 'iS', playerid);
}

self.addEventListener('playerconnect', event => {
  console.log(GetPlayerName(event.playerid) + ' joined!');
});
