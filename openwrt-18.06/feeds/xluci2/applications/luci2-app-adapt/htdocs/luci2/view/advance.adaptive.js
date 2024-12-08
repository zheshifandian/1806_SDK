L.ui.view.extend({

	get_Port: L.rpc.declare({
		object: 'luci2.advance',
		method: 'getport',
		expect: { }
	}),

	set_Port: L.rpc.declare({
		object: 'luci2.advance',
		method: 'setport',
		params: [ 'data' ],
	}),

	start_set: L.rpc.declare({
		object: 'luci2.advance',
		method: 'start',
	}),

	set_Boot: L.rpc.declare({
		object: 'luci2.advance',
		method: 'setboot',
		params: [ 'data' ],
	}),

	execute: function() {
		var self = this;

		//manual set
		var list = ["1", "2", "3", "4", "5"]
		//attention : don't use for(i=0;i<6;i++), it makes set_Port(i) always gets 6
		for(let i in list){
			$('#p'+list[i]).click(function() {
				L.ui.setting(true);
				self.set_Port(list[i]).then(function() {
					L.ui.setting(false);
					window.location.reload();
				});
			});
		}
		//auto set
		$('#start').click(function() {
			L.ui.setting(true);
			self.start_set().then(function() {
				L.ui.setting(false);
				window.location.reload();
			});
		});
		//boot auto set
		$('#boot').click(function() {
			L.ui.setting(true);
			var en = "1";
			if($('#boot').text() == "ON"){
				en = "0";
			}
			self.set_Boot(en).then(function() {
				L.ui.setting(false);
				window.location.reload();
			});
		});
		//web status
		self.get_Port().then(function(result) {
			if(result.port > 0){
				$('#p'+ result.port).text("wan");
			}
			if(result.enable != 0){
				$('#boot').text("ON");
			}
			if(result.name == "ac28"){
				$('#p5').css("display","none");
			}
		});
		return 0;
	}
});
