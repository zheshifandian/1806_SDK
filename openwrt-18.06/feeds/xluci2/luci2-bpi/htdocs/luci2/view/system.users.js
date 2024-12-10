L.ui.view.extend({
	execute: function() {
		var self = this;
		return L.ui.getAvailableACLs().then(function(acls) {
			var m = new L.cbi.Map('rpcd', {
				caption:     L.tr('Change the login password'),
				description: L.tr('Manage user account passwords and access to the WebUI interface'),
				readonly:    !self.options.acls.users
			});

			var s = m.section(L.cbi.DummySection, '__login', {
//				caption:      L.tr('Accounts'),
			});

			var password = s.option(L.cbi.PasswordValue, 'password', {
				caption:     L.tr('Password'),
				optional:    true,
				datatype:    'rangelength(5,12)'
			});

			password.ucivalue = function(sid) {
				return '';
			}
			password.toggle = function(sid) {
				var id = '#' + this.id(sid);
				this.callSuper('toggle', sid);
			};

			password.save = function(sid) {
				var real_sid;
				var rpcd_users = L.uci.sections('rpcd');
				for(var i = 0; i < rpcd_users.length; i++) {
					if(L.uci.get('rpcd', rpcd_users[i]['.name'], 'username') == 'root') {
						real_sid = rpcd_users[i]['.name'];
						break;
					}
				}

				var pw = password.formvalue(sid);

				if (!pw || !real_sid)
					return;

				return L.ui.cryptPassword(pw).then(function(crypt) {
					L.uci.set('rpcd', real_sid, 'password', crypt);
				});
			};

			m.on('save', function() {
				L.uci.changes().then(function(changes) {
					self.relogin = true;
				});
			});

			m.on('apply', function() {
				if (self.relogin) {
					location.replace('/');
					return;
				}
			});

			return m.insertInto('#map');
		});
	}
});
