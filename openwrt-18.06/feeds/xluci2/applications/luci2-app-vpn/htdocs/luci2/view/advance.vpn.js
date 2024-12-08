L.ui.view.extend({

	do_cmd: L.rpc.declare({
		object: 'web.advance',
		method: 'cmd',
		params: [ 'cmd' ]
	}),

	execute: function() {
		var self = this;

		L.uci.callLoad("openvpn").then(function(result) {
			if(result.custom_config){
				var m = new L.cbi.Map('openvpn', {
					caption:     L.tr('OPENVPN'),
					description:  L.tr('set correct config file and then start vpn')
				});

				var s = m.section(L.cbi.NamedSection, 'custom_config', {
				});

				e = s.option(L.cbi.CheckboxValue, 'enabled', {
					caption:      L.tr('Enable VPN')
				});

				s.option(L.cbi.InputValue, 'config', {
					caption:      L.tr('Config file')
				});

				e.save = function(sid){
					var value = Number(this.formvalue(sid));
					if(value == 1){
						L.uci.set("openvpn","custom_config","enabled","1");
					}else{
						L.uci.set("openvpn","custom_config","enabled","0");
					}
					L.uci.save();
					self.do_cmd("/www/luci2/scripts/set_vpn.sh " + value).then(function(){
						self.do_cmd("/etc/init.d/openvpn restart");
					});
				}

				m.insertInto('#map');
			} else {
				//not install openvpn
				var m = new L.cbi.Map('openvpn', {
					caption:     L.tr('OPENVPN'),
					description:  L.tr('not find openvpn config, please install openvpn first'),
					pageaction:  false
				});
				m.insertInto('#map');
			}
		});
	}
});
