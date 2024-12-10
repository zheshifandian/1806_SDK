L.ui.view.extend({
	cmd: L.rpc.declare({
		object: 'web.advance',
		method: 'cmd',
		params: ['cmd'],
	}),
	save: L.rpc.declare({
		object: 'file',
		method: 'write',
		params: ['path', 'data', 'append'],
	}),
	read: L.rpc.declare({
		object: 'file',
		method: 'read',
		params: ['path'],
		expect: {
			data: '',
		},
	}),
	execute: function () {
		var self = this;
		L.uci.load('feedback').then(function () {
			var smail = L.uci.get('feedback', 'mailbox', 'smail');
			var smtp = L.uci.get('feedback', 'mailbox', 'smtp');
			var suser = L.uci.get('feedback', 'mailbox', 'suser');
			var spw = L.uci.get('feedback', 'mailbox', 'spassword');
			var dmail = L.uci.get('feedback', 'mailbox', 'dmail');
			var mailfile = L.uci.get('feedback', 'mailbox', 'mailfile');
			var shfile = L.uci.get('feedback', 'mailbox', 'shfile');
			function loadbox(msg) {
				var load = $('<div />')
					.addClass('loading')
					.append(
						$('<div />')
							.addClass('loading_content absolute_center')
							.append(
								$('<div />')
									.addClass('loading_box default clearboth')
									.append(
										$('<div />').addClass('loading_gif').css({
											'margin-right': '20px',
											right: '0',
										}),
									)
									.append($('<div />').addClass('loading_word').css('height', '61px').append($('<p />').text(L.tr(msg)))),
							),
					);
				return load;
			}
			$('#box').click(function () {
				$('#feedback').focus();
			});
			$('#send').click(function () {
				if ($('#feedback').prop('innerText') != "") {
					self.save(mailfile, 'From:' + smail + '\nTo:' + dmail + '\nSubject:Feedback\n\n' + $('#feedback').prop('innerText'));
					self.save(shfile, 'sendmail -f ' + smail + ' -t ' + dmail + ' -S ' + smtp + ' -au' + suser + ' -ap' + spw + ' < ' + mailfile + '\n uci set feedback.mailbox.result="$?"');
					self.cmd('chmod +x ' + shfile);
					$('body').append(loadbox('Please wait while sending feedback email.'));
					self.cmd(shfile + '&');
					setTimeout(function () {
						var msg = 'Send failed! Please check the Settings.';
						if (L.uci.get('feedback', 'mailbox', 'result') != undefined) {
							var fail = parseInt(L.uci.get('feedback', 'mailbox', 'result'));
							if (!fail) msg = 'Sent successfully!';
							L.uci.unset('feedback', 'mailbox', 'result');
						}
						$('.loading').remove();
						$('body').append(loadbox(msg));
						$('body').find('.loading_gif').remove();
						setTimeout(function () {
							$('.loading').remove();
						}, 2000);
					}, 5000);
				} else {
					$('body').append(loadbox('Content must not be empty.'));
					$('body').find('.loading_gif').remove();
					setTimeout(function () {
						$('.loading').remove();
					}, 2000);
				}
			});
		});
	}
})
