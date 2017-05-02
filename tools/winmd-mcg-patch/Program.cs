using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;
using Mono.Cecil;
using Mono.Cecil.Cil;

[assembly: System.Runtime.CompilerServices.TypeForwardedTo(typeof(System.Runtime.InteropServices.WindowsRuntime.EventRegistrationToken))]

namespace winmd_mcg_patch
{

    public class TemplatePatchMethods
    {
        public static IntPtr GetFunctionPointer(Delegate del, out RuntimeTypeHandle typeOfFirstParameterIfInstanceDelegate)
        {
            typeOfFirstParameterIfInstanceDelegate = new RuntimeTypeHandle();
            return Marshal.GetFunctionPointerForDelegate (del);
        }
    }

    public class MyResolver : BaseAssemblyResolver
    {
        public override AssemblyDefinition Resolve(string fullName)
        {
            return base.Resolve(fullName);
        }

        public override AssemblyDefinition Resolve(AssemblyNameReference name)
        {
            return base.Resolve(name);
        }
        public override AssemblyDefinition Resolve(string fullName, ReaderParameters parameters)
        {
            return base.Resolve(fullName, parameters);
        }

        public override AssemblyDefinition Resolve(AssemblyNameReference name, ReaderParameters parameters)
        {
            return base.Resolve(name, parameters);
        }
    }

    class Program
    {
        const string templateModuleName = "winmd-mcg-patch.exe";
        const string templateNamespace = "winmd_mcg_patch";

        static void patchMethod(string baseDirectory)
        {
            ModuleDefinition templateModule = ModuleDefinition.ReadModule(templateModuleName);
            TypeDefinition templateClass = templateModule.GetType(templateNamespace + ".TemplatePatchMethods");
         
            MethodDefinition templateMethod = templateClass.Methods.OfType<MethodDefinition>()
                .Where(m => m.Name == "GetFunctionPointer").Single();

            ModuleDefinition targetModule = ModuleDefinition.ReadModule(baseDirectory + "\\Bin\\System.Private.CoreLib.InteropServices.dll");
            TypeDefinition targetClass = targetModule.GetType("System.Runtime.InteropServices.InteropExtensions");
         
            MethodDefinition targetMethod = targetClass.Methods.OfType<MethodDefinition>()
                .Where(m => m.Name == "GetFunctionPointer").Single();

            foreach (var inst in templateMethod.Body.Instructions)
            {
                Console.WriteLine (inst.ToString());
            }

            foreach (var inst in targetMethod.Body.Instructions)
            {
                Console.WriteLine (inst.ToString());
            }

            targetMethod.Body.Instructions.Clear();

            var il = targetMethod.Body.GetILProcessor();
            var initRTH = il.Create (OpCodes.Initobj, targetMethod.Module.Import(typeof(System.RuntimeTypeHandle)));
            var callDelegate = il.Create (OpCodes.Call, targetMethod.Module.Import(typeof(Marshal).GetMethod("GetFunctionPointerForDelegate", new [] { typeof(System.Delegate) })));

            foreach (var inst in templateMethod.Body.Instructions)
            {
                if (inst.OpCode == OpCodes.Initobj) {
                    targetMethod.Body.Instructions.Add (initRTH);
                } else if (inst.OpCode == OpCodes.Call) {
                    targetMethod.Body.Instructions.Add (callDelegate);
                } else {
                    targetMethod.Body.Instructions.Add (inst);
                }
            }

            targetModule.Write("System.Private.CoreLib.InteropServices.patched.dll");
        }

        static void exportType(string baseDirectory)
        {
                       
            ModuleDefinition templateModule = ModuleDefinition.ReadModule(templateModuleName);

            var resolver = new MyResolver();
            resolver.AddSearchDirectory (baseDirectory +  "\\MCG\\TargetingPack");
            resolver.AddSearchDirectory (baseDirectory + "\\Runtime");

            var parameters = new ReaderParameters(ReadingMode.Immediate);
            parameters.AssemblyResolver = resolver;

            ModuleDefinition targetModule = ModuleDefinition.ReadModule(baseDirectory + "\\Bin\\System.Private.Interop.dll", parameters);
            
            var exportType = default(ExportedType);

            // Should locate exported type template (using mscorlib) and change it to mscorlib ref in patched library.
            foreach (var templateModuleExportType in templateModule.ExportedTypes)
            {
                if (String.Compare(templateModuleExportType.Name, "EventRegistrationToken") == 0)
                {
                    exportType = templateModuleExportType;
                    break;
                }
            }
            
            foreach (var assemblyReference in targetModule.AssemblyReferences)
            {
                if (String.Compare(assemblyReference.Name, "mscorlib") == 0 && exportType != null)
                {
                    exportType.MetadataToken = new MetadataToken(TokenType.AssemblyRef, assemblyReference.MetadataToken.RID);
                    exportType.Scope.MetadataToken = new MetadataToken(TokenType.AssemblyRef, assemblyReference.MetadataToken.RID);
                    break;
                }
            }

            if (exportType != null)
            {
                targetModule.ExportedTypes.Add(exportType);
                targetModule.Write("System.Private.Interop.patched.dll");
            }
        }

        static void Main(string[] args)
        {
            string baseDirectory = "C:\\Users\\jolorens\\Documents\\Visual Studio 2015\\Projects\\winrt\\CameraAPI3";
            patchMethod (baseDirectory);
            exportType (baseDirectory);
        }
    }
}
