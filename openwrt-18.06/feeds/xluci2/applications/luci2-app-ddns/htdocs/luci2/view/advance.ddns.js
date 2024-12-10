L.ui.view.extend({

	get_ddns: L.rpc.declare({
		object: 'web.advance',
		method: 'getddns',
		expect: { }
	}),

	execute: function() {
		var self = this;

		self.get_ddns().then(function(info) {
			var ddns_status = info.result;
			if (ddns_status == 1) {

				var m = new L.cbi.Map('ddns', {
					caption:     L.tr('DDNS'),
					description:  L.tr('service provider   oray.com'),
					collabsible: true
				});

				var s = m.section(L.cbi.NamedSection, 'myddns_ipv4', {
					caption:      L.tr('Peanut Shell Dynamic Domain Name'),
					teasers:      ['username', 'password', '__auto', 'domain' ]
				});

				e = s.option(L.cbi.InputValue, 'username', {
					caption:      L.tr('Username')
				});

				e = s.option(L.cbi.PasswordValue, 'password', {
					caption:    L.tr('Password'),
					datatype:   'password'
				});

				e = s.option(L.cbi.CheckboxValue, '__auto', {
					caption:      L.tr('auto login')
				});

				e.ucivalue = function(sid){
					var v = L.uci.get("ddns","myddns_ipv4","interface");
					if(v == "wan"){
						return true;
					}
					else{
						return false;
					}
				}

				e.save = function(sid){
					var value = Number(this.formvalue(sid));
					if(value == 1){
						L.uci.set("ddns","myddns_ipv4","interface","wan");
					}else{
						L.uci.set("ddns","myddns_ipv4","interface", "");
					}
					L.uci.save();
				}

				e = s.option(L.cbi.InputValue, 'domain', {
					caption:        L.tr('Domain Information')
				});

				m.insertInto('#map');
			}

			else {
				 var m = new L.cbi.Map('ddns', {
                                        caption:     L.tr('DDNS'),
                                        description:  L.tr('ddns related script is not detected, please select in menuconfig'),
					pageaction:  false
                                });
				m.insertInto('#map');
                }
		})
	}
});
