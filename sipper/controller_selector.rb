# If the order.yaml does not have any controller then 
# the cotrollers defined in the controllers directory are taken
# in the alphabetical order, otherwise the order defined in 
# the yaml is taken for the ones present, the rest are arranged alphabetically. 
# There can be an optional block in the yaml file which is invoked to see 
# if the controller should be invoked or not. Much like a rules engine.
# If the controller doesnt want to handle a request, it returns false, true otherwise.
# Data structure to check for on request 
#  - [[fname, <controller_instance>, <block/proc>],..] ordered by order in yaml and then alphabetical
#  Iteration-1 only ordered instance array, later add block

require 'request'
require 'yaml'
require 'sip_logger'
require 'util/sipper_util'
require 'controller_class_loader'

module SIP
  class ControllerSelector
    include SipLogger
    include SipperUtil
    
    # this goes off and creates the controlllers and maintain
    # the order map 
    def initialize dir
      @controllers = {} #  fqname=> instance 
      @load_dir = dir
      load_controllers dir  if @load_dir
    end
    
    # Load all or a defined controller
    # todo code to load a named controller
    def load_controllers(dir=@load_dir, controller=nil)
      dir = Dir.new(dir) unless dir.class <= Dir
      order_arr = nil
      dir.each do |f|
        next if File.directory?(f)
        path = File.join(dir.path, f)
        logd("Path is #{path}")
        if f=~/order.yaml/
          @order_arr = SipperUtil.load_yaml(path)
        elsif f=~/controller.rb$/
          SIP::ControllerClassLoader.load( path )
        end
      end
      create_controllers
      logd("There are a total of #{@controllers.size} controllers, they are #{@controllers.keys.join(',')}")
    end
    
   
    def load_controller_from_string(str)
      existing_controllers = SIP::ControllerClassLoader.controllers
      SIP::ControllerClassLoader.load_from_string(str)
      new_controllers = SIP::ControllerClassLoader.controllers - existing_controllers
      name = nil
      new_controllers.each do |fqcname|
        create_controller( fqcname )
        name = fqcname
      end
      name
    end
   
    def get_controllers(request=nil)
      if @order_arr
        logd("order_arr present with #{@order_arr.join(',')}")
        control_order = @controllers.sort do |x,y|
          # todo DRY it when you have tests
          (@order_arr.index(x[0])?@order_arr.index(x[0]):@order_arr.length) <=> (@order_arr.index(y[0])?@order_arr.index(y[0]):@order_arr.length)
        end 
      else
        control_order = @controllers.sort
      end
      control_order.map {|x| x[1]}
    end
      
    def get_controller(name)
      @controllers[name]
    end
  
  
    def clear_all
      @controllers = {}
      @order_arr = nil
      SIP::ControllerClassLoader.clear_all 
    end
  
   
    
    def create_controllers
      logd("Loading controller from ControllerClassFinder")
      SIP::ControllerClassLoader.controllers.each do |fqcname|
        create_controller(fqcname)  
      end
    end
    
    def create_controller(fqcname)
      m = SipperUtil.constantize(fqcname)       
      if m.class == Class
        @controllers[fqcname] = inst = m.new  
        if inst.order >= 0
          if @order_arr
            @order_arr.insert([inst.order, @order_arr.length].min, fqcname) 
          else
            @order_arr = [fqcname]
          end
        end
      end
    end
    
    private  :create_controllers 
    
  end
end

if $PROGRAM_NAME == __FILE__
  cs = SIP::ControllerSelector.new(Dir.new(File.join(SipperConfigurator[:SipperBasePath], "sipper", "controllers")))
  ctr = cs.get_controllers(nil)
  ctr.each {|x| p x.name}  
end