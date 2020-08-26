using MQTTnet;
using MQTTnet.Client;
using MQTTnet.Client.Connecting;
using MQTTnet.Client.Options;
using MQTTnet.Client.Subscribing;
using MQTTnet.Formatter;
using System;
using System.Diagnostics;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace MqttClient
{
    public partial class FeedForm : Form
    {
        public IMqttClient Client { get; private set; }
        public ConnectionForm connectionForm;
        public AboutForm autoForm;
        public FeedForm()
        {
            InitializeComponent();

            Client = new MqttFactory().CreateMqttClient();
            connectionForm = new ConnectionForm();
            autoForm = new AboutForm();
        }

        private async void btnStartTerminal_Click(object sender, EventArgs e)
        {
            try
            {
                if(btnSubscribe.Text == "Start")
                {
                    IMqttClientOptions options = new MqttClientOptionsBuilder()
                        .WithTcpServer(connectionForm.GetHost(), connectionForm.GetPort())
                        .WithCredentials(connectionForm.GetUserName(), connectionForm.GetPassword())
                        .WithProtocolVersion(MqttProtocolVersion.V311)
                        .Build();

                    var auth = await Client.ConnectAsync(options);
                    if (auth.ResultCode != MqttClientConnectResultCode.Success)
                    {
                        throw new Exception(auth.ResultCode.ToString());
                    }

                    btnSubscribe.Text = "Stop";
                    txtTopic.Enabled = false;

                    var result = (await Client.SubscribeAsync(
                      new TopicFilterBuilder()
                      .WithTopic(txtTopic.Text)
                      .Build()
                  )).Items[0];

                    switch (result.ResultCode)
                    {
                        case MqttClientSubscribeResultCode.GrantedQoS0:
                        case MqttClientSubscribeResultCode.GrantedQoS1:
                        case MqttClientSubscribeResultCode.GrantedQoS2:

                            Client.UseApplicationMessageReceivedHandler(me =>
                            {
                                var msg = me.ApplicationMessage;
                                var data = Encoding.UTF8.GetString(msg.Payload);

                                Invoke((Action)(() =>
                                {
                                    txtStream.AppendText($"{data}\n");
                                    txtStream.ScrollToCaret();
                                    txtStream.Refresh();
                                }));
                            });

                            break;
                        default:
                            throw new Exception(result.ResultCode.ToString());
                    }
                }
                else
                {
                    btnSubscribe.Text = "Start";
                    txtTopic.Enabled = true;
                    
                    var result = (await Client.UnsubscribeAsync(txtTopic.Text)).Items[0];
                    await Client.DisconnectAsync();
                    switch (result.ReasonCode)
                    {
                        case MQTTnet.Client.Unsubscribing.MqttClientUnsubscribeResultCode.Success:
                            break;
                        default:
                            throw new Exception(result.ReasonCode.ToString());
                    }
                }
                
            }
            catch(Exception ex)
            {
                txtTopic.Enabled = true;
                btnSubscribe.Enabled = true;
                this.Error(ex);
            }
        }
        private bool ToggleStatus { get; set; }

        private async Task BtnToggle_Click(object sender, EventArgs e)
        {
            try
            {

                await Client.PublishAsync(
                        txtTopic.Text, 
                        (ToggleStatus = ! ToggleStatus) ? "ON" : "OFF"
                    );
            }
            catch (Exception ex)
            {
                this.Error(ex);
            }

        }

        private void button1_Click(object sender, EventArgs e)
        {
            txtStream.Text = "";
        }

        private void btnServerSettings_Click(object sender, EventArgs e)
        {
            connectionForm.ShowDialog();
        }

        private void aboutToolStripMenuItem_Click(object sender, EventArgs e)
        {
            autoForm.ShowDialog();
        }
    }
}
