using MQTTnet;
using MQTTnet.Client;
using MQTTnet.Client.Connecting;
using MQTTnet.Client.Options;
using MQTTnet.Formatter;
using System;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace MqttClient
{
    public partial class ConnectionForm : Form
    {
        public ConnectionForm()
        {
            InitializeComponent();
        }
        public String GetHost()
        {
            try
            {
                return txtHost.Text;
            }
            catch (Exception)
            {

            }
            return "";
        }

        public int GetPort()
        {
            try
            {
                return int.Parse(txtPort.Text);
            }
            catch (Exception)
            {

            }
            return 0;
        }

        public String GetUserName()
        {
            try
            {
                return txtUsername.Text;
            }
            catch (Exception)
            {

            }
            return "";
        }

        public String GetPassword()
        {
            try
            {
                return txtPassword.Text;
            }
            catch (Exception)
            {

            }
            return "";
        }

        private async void BtnConnect_ClickAsync(object sender, System.EventArgs e)
        {
            Hide();
        }
        
    }
}
