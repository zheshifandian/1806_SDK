L.ui.view.extend({

	do_cmd: L.rpc.declare({
		object: 'web.advance',
		method: 'cmd',
		params: [ 'cmd' ]
	}),

	execute: function() {
		var self = this;

		L.uci.callLoad("smartdns").then(function(result) {
			if(!$.isEmptyObject(result)){
				var m = new L.cbi.Map('smartdns', {
					caption:     L.tr('SmartDNS'),
					description:  L.tr('SmartDNS is a local DNS server. SmartDNS accepts DNS query requests from local clients, obtains DNS query results from multiple upstream DNS servers, and returns the fastest access results to clients.')
				});

				var s = m.section(L.cbi.TypedSection, 'smartdns', {
				});

				e = s.option(L.cbi.CheckboxValue, 'enabled', {
					caption:      L.tr('Enable SmartDNS')
				});

				e.save = function(sid){
					var value = Number(this.formvalue(sid));
					var smartdns_id = L.uci.sections('smartdns')[0]['.name'];
					if(value == 1){
						L.uci.set("smartdns", smartdns_id, "enabled", "1");
					}else{
						L.uci.set("smartdns", smartdns_id, "enabled", "0");
					}
					L.uci.save().then(function(){
						self.do_cmd("sleep 1;/etc/init.d/smartdns restart");
					});
				}

				m.insertInto('#map');
			} else {
				//not install openvpn
				var m = new L.cbi.Map('smartdns', {
					caption:     L.tr('SmartDNS'),
					description:  L.tr('not find SmartDNS config, please install SmartDNS first'),
					pageaction:  false
				});
				m.insertInto('#map');
			}
		});
	}
});
